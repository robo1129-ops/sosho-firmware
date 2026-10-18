#include <zephyr/kernel.h>
#include <zephyr/devicetree.h>
#include <zephyr/settings/settings.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/gatt.h>
#include <zephyr/bluetooth/uuid.h>
#include <zephyr/sys/byteorder.h>

#include <zmk/behavior.h>
#include <zmk/behavior_queue.h>

#include <errno.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

/*
 * SO-SHO runtime macro extension.
 *
 * This is deliberately separate from the official ZMK Studio protobuf/RPC
 * surface. A macro is stored as one validated record and can be changed at
 * runtime over an encrypted BLE connection; the firmware does not need to be
 * rebuilt for each macro edit.
 */
#define SOSHO_MACRO_SERVICE_UUID \
    BT_UUID_DECLARE_128(BT_UUID_128_ENCODE(0x7f5e0000, 0x8a2a, 0x4c2f, 0xb93e, 0x55f4b5d30001))
#define SOSHO_MACRO_COMMAND_UUID \
    BT_UUID_DECLARE_128(BT_UUID_128_ENCODE(0x7f5e0001, 0x8a2a, 0x4c2f, 0xb93e, 0x55f4b5d30001))
#define SOSHO_MACRO_RESPONSE_UUID \
    BT_UUID_DECLARE_128(BT_UUID_128_ENCODE(0x7f5e0002, 0x8a2a, 0x4c2f, 0xb93e, 0x55f4b5d30001))

#define SOSHO_MAGIC0 0x53
#define SOSHO_MAGIC1 0x53
#define SOSHO_MAGIC2 0x48
#define SOSHO_MAGIC3 0x4f
#define SOSHO_PROTOCOL_VERSION 1

#define SOSHO_OP_HELLO 1
#define SOSHO_OP_UNLOCK 2
#define SOSHO_OP_GET 3
#define SOSHO_OP_SET 4
#define SOSHO_OP_DELETE 5
#define SOSHO_OP_RUN 6
#define SOSHO_OP_SET_BEGIN 7
#define SOSHO_OP_SET_CHUNK 8
#define SOSHO_OP_SET_COMMIT 9

#define SOSHO_STATUS_OK 0
#define SOSHO_STATUS_BAD_FRAME 1
#define SOSHO_STATUS_BAD_CRC 2
#define SOSHO_STATUS_AUTH_REQUIRED 3
#define SOSHO_STATUS_TOO_LARGE 4
#define SOSHO_STATUS_BAD_VALUE 5

#define SOSHO_MAX_MACROS 16
#define SOSHO_MAX_PAYLOAD 224
#define SOSHO_MAX_STEPS 8
#define SOSHO_MAX_WAIT_MS 60000U
#define SOSHO_STEP_SIZE 7
#define SOSHO_WIRE_PAYLOAD_MAX 8
#define SOSHO_CHUNK_DATA 3

#define SOSHO_KEY_PRESS_BEHAVIOR DEVICE_DT_NAME(DT_INST(0, zmk_behavior_key_press))

static uint8_t macro_blob[SOSHO_MAX_MACROS][SOSHO_MAX_PAYLOAD];
static uint16_t macro_blob_len[SOSHO_MAX_MACROS];
static uint8_t pending_blob[SOSHO_MAX_PAYLOAD];
static uint16_t pending_blob_len;
static uint16_t pending_blob_received;
static uint8_t pending_blob_id;
static bool pending_blob_active;
static bool studio_unlocked;
static uint8_t response[12 + SOSHO_MAX_PAYLOAD];
static uint16_t response_len;

static const char *const macro_settings_keys[SOSHO_MAX_MACROS] = {
    "sosho_macro/0", "sosho_macro/1", "sosho_macro/2", "sosho_macro/3",
    "sosho_macro/4", "sosho_macro/5", "sosho_macro/6", "sosho_macro/7",
    "sosho_macro/8", "sosho_macro/9", "sosho_macro/10", "sosho_macro/11",
    "sosho_macro/12", "sosho_macro/13", "sosho_macro/14", "sosho_macro/15",
};

static uint16_t crc16(const uint8_t *data, size_t len) {
    uint16_t crc = 0xffff;

    for (size_t i = 0; i < len; i++) {
        crc ^= data[i];
        for (uint8_t bit = 0; bit < 8; bit++) {
            crc = (crc & 1U) ? (crc >> 1) ^ 0xa001 : (crc >> 1);
        }
    }

    return crc;
}

static int macro_id_from_name(const char *name) {
    char *end = NULL;
    unsigned long value;

    if (name == NULL || name[0] == '\0') {
        return -EINVAL;
    }

    value = strtoul(name, &end, 10);
    if (end == name || *end != '\0' || value >= SOSHO_MAX_MACROS) {
        return -EINVAL;
    }

    return (int)value;
}

static int settings_set(const char *name, size_t len_rd, settings_read_cb read_cb, void *cb_arg) {
    int id = macro_id_from_name(name);
    if (id < 0) {
        return -ENOENT;
    }
    if (len_rd > SOSHO_MAX_PAYLOAD) {
        return -EINVAL;
    }

    ssize_t read = read_cb(cb_arg, macro_blob[id], sizeof(macro_blob[id]));
    if (read < 0) {
        return (int)read;
    }

    macro_blob_len[id] = (uint16_t)read;
    return 0;
}

SETTINGS_STATIC_HANDLER_DEFINE(sosho_macro, "sosho_macro", NULL, settings_set, NULL, NULL);

static bool valid_macro_blob(const uint8_t *data, uint16_t len, uint8_t expected_id) {
    if (data == NULL || len < 4 || len > SOSHO_MAX_PAYLOAD || data[0] != expected_id) {
        return false;
    }

    uint8_t steps = data[1];
    uint8_t name_len = data[2];
    uint8_t target_len = data[3];
    if (steps > SOSHO_MAX_STEPS || name_len > 24 || target_len > 32) {
        return false;
    }

    uint16_t header_len = (uint16_t)(4U + name_len + target_len);
    uint16_t expected_len = (uint16_t)(header_len + (steps * SOSHO_STEP_SIZE));
    if (expected_len != len) {
        return false;
    }

    const uint8_t *step = &data[header_len];
    for (uint8_t i = 0; i < steps; i++, step += SOSHO_STEP_SIZE) {
        uint8_t mode = step[4];
        uint16_t wait = sys_get_le16(&step[5]);
        if (mode > 2 || wait > SOSHO_MAX_WAIT_MS) {
            return false;
        }
    }

    return true;
}

static void send_response(struct bt_conn *conn, uint8_t op, uint8_t txid, uint8_t status,
                          const uint8_t *data, uint16_t len);

static void response_ccc_changed(const struct bt_gatt_attr *attr, uint16_t value) {
    ARG_UNUSED(attr);
    ARG_UNUSED(value);
}

static int run_macro(uint8_t id) {
    if (id >= SOSHO_MAX_MACROS || macro_blob_len[id] == 0) {
        return -ENOENT;
    }

    const struct device *key_press = zmk_behavior_get_binding(SOSHO_KEY_PRESS_BEHAVIOR);
    if (key_press == NULL || !device_is_ready(key_press)) {
        return -ENODEV;
    }

    const uint8_t *data = macro_blob[id];
    uint8_t steps = data[1];
    uint8_t name_len = data[2];
    uint8_t target_len = data[3];
    const uint8_t *step = &data[4 + name_len + target_len];
    struct zmk_behavior_binding_event event = {
        .position = UINT32_MAX,
        .timestamp = k_uptime_get(),
    };
    struct zmk_behavior_binding binding = {
        .behavior_dev = SOSHO_KEY_PRESS_BEHAVIOR,
        .param1 = 0,
        .param2 = 0,
    };

    for (uint8_t i = 0; i < steps; i++, step += SOSHO_STEP_SIZE) {
        binding.param1 = sys_get_le32(step);
        uint8_t mode = step[4];
        uint16_t wait = sys_get_le16(&step[5]);

        if (mode == 0) {
            if (zmk_behavior_queue_add(&event, binding, true, wait) < 0 ||
                zmk_behavior_queue_add(&event, binding, false, 0) < 0) {
                return -ENOSPC;
            }
        } else if (mode == 1) {
            if (zmk_behavior_queue_add(&event, binding, true, wait) < 0) {
                return -ENOSPC;
            }
        } else {
            if (zmk_behavior_queue_add(&event, binding, false, wait) < 0) {
                return -ENOSPC;
            }
        }
    }

    return 0;
}

static ssize_t command_write(struct bt_conn *conn, const struct bt_gatt_attr *attr,
                             const void *buf, uint16_t len, uint16_t offset, uint8_t flags) {
    ARG_UNUSED(attr);
    ARG_UNUSED(flags);

    const uint8_t *packet = buf;
    if (offset != 0 || len < 12 || len > sizeof(response)) {
        return -EINVAL;
    }

    if (packet[0] != SOSHO_MAGIC0 || packet[1] != SOSHO_MAGIC1 || packet[2] != SOSHO_MAGIC2 ||
        packet[3] != SOSHO_MAGIC3 || packet[4] != SOSHO_PROTOCOL_VERSION) {
        return -EPROTO;
    }

    uint16_t payload_len = sys_get_le16(&packet[8]);
    if ((uint16_t)(12U + payload_len) != len || payload_len > SOSHO_MAX_PAYLOAD) {
        send_response(conn, packet[5], packet[6], SOSHO_STATUS_BAD_FRAME, NULL, 0);
        return len;
    }

    uint16_t expected_crc = sys_get_le16(&packet[10 + payload_len]);
    if (crc16(packet, 10 + payload_len) != expected_crc) {
        send_response(conn, packet[5], packet[6], SOSHO_STATUS_BAD_CRC, NULL, 0);
        return len;
    }

    uint8_t op = packet[5];
    uint8_t txid = packet[6];
    const uint8_t *payload = &packet[10];

    if (op == SOSHO_OP_HELLO) {
        uint8_t info[6] = {SOSHO_PROTOCOL_VERSION, SOSHO_MAX_MACROS, 0, 0, 0, 0};
        sys_put_le16(SOSHO_MAX_PAYLOAD, &info[2]);
        sys_put_le16(SOSHO_MAX_WAIT_MS, &info[4]);
        send_response(conn, op, txid, SOSHO_STATUS_OK, info, sizeof(info));
    } else if (bt_conn_get_security(conn) < BT_SECURITY_L2) {
        send_response(conn, op, txid, SOSHO_STATUS_AUTH_REQUIRED, NULL, 0);
    } else if (op == SOSHO_OP_UNLOCK) {
        studio_unlocked = payload_len == 0;
        send_response(conn, op, txid,
                      studio_unlocked ? SOSHO_STATUS_OK : SOSHO_STATUS_AUTH_REQUIRED, NULL, 0);
    } else if (!studio_unlocked) {
        send_response(conn, op, txid, SOSHO_STATUS_AUTH_REQUIRED, NULL, 0);
    } else if (op == SOSHO_OP_GET) {
        if ((payload_len != 1 && payload_len != 3) || payload[0] >= SOSHO_MAX_MACROS) {
            send_response(conn, op, txid, SOSHO_STATUS_BAD_VALUE, NULL, 0);
        } else {
            uint8_t id = payload[0];
            if (payload_len == 1) {
                send_response(conn, op, txid, SOSHO_STATUS_OK, macro_blob[id], macro_blob_len[id]);
            } else {
                uint16_t offset = sys_get_le16(&payload[1]);
                uint16_t total = macro_blob_len[id];
                if (offset > total) {
                    send_response(conn, op, txid, SOSHO_STATUS_BAD_VALUE, NULL, 0);
                } else {
                    uint16_t remaining = total - offset;
                    uint8_t chunk = (uint8_t)MIN(remaining, SOSHO_CHUNK_DATA);
                    uint8_t response_data[5 + SOSHO_CHUNK_DATA] = {id, 0, 0, 0, 0, 0, 0, 0};
                    sys_put_le16(offset, &response_data[1]);
                    sys_put_le16(total, &response_data[3]);
                    if (chunk > 0) {
                        memcpy(&response_data[5], &macro_blob[id][offset], chunk);
                    }
                    send_response(conn, op, txid, SOSHO_STATUS_OK, response_data, 5 + chunk);
                }
            }
        }
    } else if (op == SOSHO_OP_SET) {
        if (payload_len < 1 || !valid_macro_blob(&payload[1], payload_len - 1, payload[0])) {
            send_response(conn, op, txid, SOSHO_STATUS_BAD_VALUE, NULL, 0);
        } else {
            uint8_t id = payload[0];
            uint16_t blob_len = payload_len - 1;
            memcpy(macro_blob[id], &payload[1], blob_len);
            macro_blob[id][0] = id;
            macro_blob_len[id] = blob_len;
            int err = settings_save_one(macro_settings_keys[id], macro_blob[id], blob_len);
            send_response(conn, op, txid, err ? SOSHO_STATUS_BAD_VALUE : SOSHO_STATUS_OK, NULL, 0);
        }
    } else if (op == SOSHO_OP_SET_BEGIN) {
        if (payload_len != 3 || payload[0] >= SOSHO_MAX_MACROS) {
            send_response(conn, op, txid, SOSHO_STATUS_BAD_VALUE, NULL, 0);
        } else {
            uint16_t total = sys_get_le16(&payload[1]);
            if (total < 4 || total > SOSHO_MAX_PAYLOAD) {
                send_response(conn, op, txid, SOSHO_STATUS_TOO_LARGE, NULL, 0);
            } else {
                pending_blob_id = payload[0];
                pending_blob_len = total;
                pending_blob_received = 0;
                pending_blob_active = true;
                send_response(conn, op, txid, SOSHO_STATUS_OK, NULL, 0);
            }
        }
    } else if (op == SOSHO_OP_SET_CHUNK) {
        if (!pending_blob_active || payload_len < 4 || payload_len > 3 + SOSHO_CHUNK_DATA ||
            payload[0] != pending_blob_id || sys_get_le16(&payload[1]) != pending_blob_received ||
            (uint16_t)(pending_blob_received + payload_len - 3) > pending_blob_len) {
            send_response(conn, op, txid, SOSHO_STATUS_BAD_VALUE, NULL, 0);
        } else {
            uint16_t chunk_len = payload_len - 3;
            memcpy(&pending_blob[pending_blob_received], &payload[3], chunk_len);
            pending_blob_received += chunk_len;
            send_response(conn, op, txid, SOSHO_STATUS_OK, NULL, 0);
        }
    } else if (op == SOSHO_OP_SET_COMMIT) {
        if (!pending_blob_active || payload_len != 1 || payload[0] != pending_blob_id ||
            pending_blob_received != pending_blob_len ||
            !valid_macro_blob(pending_blob, pending_blob_len, pending_blob_id)) {
            send_response(conn, op, txid, SOSHO_STATUS_BAD_VALUE, NULL, 0);
        } else {
            uint8_t id = pending_blob_id;
            memcpy(macro_blob[id], pending_blob, pending_blob_len);
            macro_blob_len[id] = pending_blob_len;
            int err = settings_save_one(macro_settings_keys[id], macro_blob[id], macro_blob_len[id]);
            pending_blob_active = false;
            send_response(conn, op, txid, err ? SOSHO_STATUS_BAD_VALUE : SOSHO_STATUS_OK, NULL, 0);
        }
    } else if (op == SOSHO_OP_DELETE) {
        if (payload_len != 1 || payload[0] >= SOSHO_MAX_MACROS) {
            send_response(conn, op, txid, SOSHO_STATUS_BAD_VALUE, NULL, 0);
        } else {
            uint8_t id = payload[0];
            macro_blob_len[id] = 0;
            int err = settings_delete(macro_settings_keys[id]);
            send_response(conn, op, txid, err ? SOSHO_STATUS_BAD_VALUE : SOSHO_STATUS_OK, NULL, 0);
        }
    } else if (op == SOSHO_OP_RUN) {
        int err = (payload_len == 1) ? run_macro(payload[0]) : -EINVAL;
        send_response(conn, op, txid,
                      err == 0 ? SOSHO_STATUS_OK : SOSHO_STATUS_BAD_VALUE, NULL, 0);
    } else {
        send_response(conn, op, txid, SOSHO_STATUS_BAD_VALUE, NULL, 0);
    }

    return len;
}

static ssize_t response_read(struct bt_conn *conn, const struct bt_gatt_attr *attr, void *buf,
                             uint16_t len, uint16_t offset) {
    return bt_gatt_attr_read(conn, attr, buf, len, offset, response, response_len);
}

BT_GATT_SERVICE_DEFINE(
    sosho_macro_svc, BT_GATT_PRIMARY_SERVICE(SOSHO_MACRO_SERVICE_UUID),
    BT_GATT_CHARACTERISTIC(SOSHO_MACRO_COMMAND_UUID,
                           BT_GATT_CHRC_WRITE | BT_GATT_CHRC_WRITE_WITHOUT_RESP,
                           BT_GATT_PERM_WRITE_ENCRYPT, NULL, command_write, NULL),
    BT_GATT_CHARACTERISTIC(SOSHO_MACRO_RESPONSE_UUID, BT_GATT_CHRC_READ | BT_GATT_CHRC_NOTIFY,
                           BT_GATT_PERM_READ_ENCRYPT, response_read, NULL, NULL),
    BT_GATT_CCC(response_ccc_changed, BT_GATT_PERM_READ_ENCRYPT | BT_GATT_PERM_WRITE_ENCRYPT));

static void send_response(struct bt_conn *conn, uint8_t op, uint8_t txid, uint8_t status,
                          const uint8_t *data, uint16_t len) {
    if (len > SOSHO_WIRE_PAYLOAD_MAX) {
        len = 0;
        status = SOSHO_STATUS_TOO_LARGE;
    }

    response[0] = SOSHO_MAGIC0;
    response[1] = SOSHO_MAGIC1;
    response[2] = SOSHO_MAGIC2;
    response[3] = SOSHO_MAGIC3;
    response[4] = SOSHO_PROTOCOL_VERSION;
    response[5] = op;
    response[6] = txid;
    response[7] = status;
    sys_put_le16(len, &response[8]);
    if (len > 0 && data != NULL) {
        memcpy(&response[10], data, len);
    }
    sys_put_le16(crc16(response, 10 + len), &response[10 + len]);
    response_len = 12 + len;
    (void)bt_gatt_notify(conn, &sosho_macro_svc.attrs[4], response, response_len);
}
