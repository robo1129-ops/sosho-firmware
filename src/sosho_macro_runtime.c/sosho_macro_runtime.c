#include <zephyr/kernel.h>
#include <zephyr/settings/settings.h>
#include <zephyr/bluetooth/gatt.h>
#include <zephyr/bluetooth/uuid.h>
#include <zephyr/sys/byteorder.h>
#include <errno.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>

/* SO-SHO runtime macro extension. This is intentionally separate from the
 * official ZMK Studio protobuf/RPC surface. */
#define SOSHO_MACRO_SERVICE_UUID BT_UUID_DECLARE_128(BT_UUID_128_ENCODE(0x7f5e0000, 0x8a2a, 0x4c2f, 0xb93e, 0x55f4b5d30001))
#define SOSHO_MACRO_COMMAND_UUID BT_UUID_DECLARE_128(BT_UUID_128_ENCODE(0x7f5e0001, 0x8a2a, 0x4c2f, 0xb93e, 0x55f4b5d30001))
#define SOSHO_MACRO_RESPONSE_UUID BT_UUID_DECLARE_128(BT_UUID_128_ENCODE(0x7f5e0002, 0x8a2a, 0x4c2f, 0xb93e, 0x55f4b5d30001))
#define SOSHO_MAGIC0 0x53
#define SOSHO_MAGIC1 0x53
#define SOSHO_MAGIC2 0x48
#define SOSHO_MAGIC3 0x4f
#define SOSHO_VERSION 1
#define SOSHO_OP_HELLO 1
#define SOSHO_OP_UNLOCK 2
#define SOSHO_OP_GET 3
#define SOSHO_OP_SET 4
#define SOSHO_OP_DELETE 5
#define SOSHO_OP_RUN 6
#define SOSHO_STATUS_OK 0
#define SOSHO_STATUS_BAD_FRAME 1
#define SOSHO_STATUS_BAD_CRC 2
#define SOSHO_STATUS_AUTH_REQUIRED 3
#define SOSHO_STATUS_TOO_LARGE 4
#define SOSHO_STATUS_BAD_VALUE 5
#define SOSHO_STATUS_NOT_IMPLEMENTED 6
#define SOSHO_MAX_PAYLOAD 224
#define SOSHO_MAX_STEPS 8
#define SOSHO_MAX_WAIT_MS 60000U
#define SOSHO_SETTINGS_KEY "sosho_macro/blob"

static uint8_t macro_blob[SOSHO_MAX_PAYLOAD];
static uint16_t macro_blob_len;
static bool studio_unlocked;
static uint8_t response[12 + SOSHO_MAX_PAYLOAD];
static uint16_t response_len;
static struct bt_gatt_ccc_cfg response_ccc[BT_GATT_CCC_MAX];

static uint16_t crc16(const uint8_t *data, size_t len)
{
    uint16_t crc = 0xffff;
    for (size_t i = 0; i < len; i++) {
        crc ^= data[i];
        for (uint8_t bit = 0; bit < 8; bit++) {
            crc = (crc & 1U) ? (crc >> 1) ^ 0xa001 : (crc >> 1);
        }
    }
    return crc;
}

static void send_response(struct bt_conn *conn, uint8_t op, uint8_t txid,
                          uint8_t status, const uint8_t *data, uint16_t len)
{
    if (len > SOSHO_MAX_PAYLOAD - 4) {
        len = 0;
        status = SOSHO_STATUS_TOO_LARGE;
    }
    response[0] = SOSHO_MAGIC0;
    response[1] = SOSHO_MAGIC1;
    response[2] = SOSHO_MAGIC2;
    response[3] = SOSHO_MAGIC3;
    response[4] = SOSHO_VERSION;
    response[5] = op;
    response[6] = txid;
    response[7] = status;
    sys_put_le16(len, &response[8]);
    if (len && data) {
        memcpy(&response[10], data, len);
    }
    sys_put_le16(crc16(response, 10 + len), &response[10 + len]);
    response_len = 12 + len;
    (void)bt_gatt_notify(conn, &sosho_macro_svc.attrs[2], response, response_len);
}

static int settings_set(const char *name, size_t len_rd,
                        settings_read_cb read_cb, void *cb_arg)
{
    if (strcmp(name, "blob") != 0) {
        return -ENOENT;
    }
    if (len_rd > sizeof(macro_blob)) {
        return -EINVAL;
    }
    ssize_t read = read_cb(cb_arg, macro_blob, sizeof(macro_blob));
    if (read < 0) {
        return (int)read;
    }
    macro_blob_len = (uint16_t)read;
    return 0;
}

SETTINGS_STATIC_HANDLER_DEFINE(sosho_macro, "sosho_macro", NULL,
                              settings_set, NULL, NULL);

static bool valid_blob(const uint8_t *data, uint16_t len)
{
    if (!data || len < 4 || len > SOSHO_MAX_PAYLOAD) { 
        return false;
    }
    /* Blob header: macro id, step count, name length, target length. */
    uint8_t steps = data[1];
    uint8_t name_len = data[2];
    uint8_t target_len = data[3];
    if (steps > SOSHO_MAX_STEPS || name_len > 24 || target_len > 32) {
        return false;
    }
    if ((uint16_t)(4 + name_len + target_len + (steps * 7U)) > len) {
        return false;
    }
    const uint8_t *step = &data[4 + name_len + target_len];
    for (uint8_t i = 0; i < steps; i++, step += 7) {
        uint8_t mode = step[2];
        uint32_t wait = sys_get_le32(&step[3]);
        if (mode > 2 || wait > SOSHO_MAX_WAIT_MS) {
            return false;
        }
    }
    return true;
}

static ssize_t command_write(struct bt_conn *conn,
                             const struct bt_gatt_attr *attr,
                             const void *buf, uint16_t len, uint16_t offset,
                             uint8_t flags)
{
    ARG_UNUSED(attr);
    ARG_UNUSED(flags);
    const uint8_t *packet = buf;
    if (offset != 0 || len < 12 || len > 256) {
        return -EINVAL;
    }
    if (packet[0] != SOSHO_MAGIC0 || packet[1] != SOSHO_MAGIC1 ||
        packet[2] != SOSHO_MAGIC2 || packet[3] != SOSHO_MAGIC3 ||
        packet[4] != SOSHO_VERSION) {
        return -EPROTO;
    }
    uint16_t payload_len = sys_get_le16(&packet[8]);
    if ((uint16_t)(12 + payload_len) != len || payload_len > SOSHO_MAX_PAYLOAD) {
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
        uint8_t info[4] = { SOSHO_VERSION, SOSHO_MAX_STEPS, 0, 0 };
        sys_put_le16(SOSHO_MAX_WAIT_MS, &info[2]);
        send_response(conn, op, txid, SOSHO_STATUS_OK, info, sizeof(info));
    } else if (op == SOSHO_OP_UNLOCK) {
        studio_unlocked = payload_len == 0;
        send_response(conn, op, txid, studio_unlocked ? SOSHO_STATUS_OK : SOSHO_STATUS_AUTH_REQUIRED, NULL, 0);
    } else if (!studio_unlocked) {
        send_response(conn, op, txid, SOSHO_STATUS_AUTH_REQUIRED, NULL, 0);
    } else if (op == SOSHO_OP_GET) {
        send_response(conn, op, txid, SOSHO_STATUS_OK, macro_blob, macro_blob_len);
    } else if (op == SOSHO_OP_SET) {
        if (!valid_blob(payload, payload_len)) {
            send_response(conn, op, txid, SOSHO_STATUS_BAD_VALUE, NULL, 0);
        } else {
            memcpy(macro_blob, payload, payload_len);
            macro_blob_len = payload_len;
            int err = settings_save_one(SOSHO_SETTINGS_KEY, macro_blob, macro_blob_len);
            send_response(conn, op, txid, err ? SOSHO_STATUS_BAD_VALUE : SOSHO_STATUS_OK, NULL, 0);
        }
    } else if (op == SOSHO_OP_DELETE) {
        macro_blob_len = 0;
        int err = settings_save_one(SOSHO_SETTINGS_KEY, macro_blob, 0);
        send_response(conn, op, txid, err ? SOSHO_STATUS_BAD_VALUE : SOSHO_STATUS_OK, NULL, 0);
    } else if (op == SOSHO_OP_RUN) {
        /* Storage and validation are implemented; ZMK key-event injection is
         * deliberately gated until a behavior-queue integration is verified. */
        send_response(conn, op, txid, SOSHO_STATUS_NOT_IMPLEMENTED, NULL, 0);
    } else {
        send_response(conn, op, txid, SOSHO_STATUS_BAD_VALUE, NULL, 0);
    }
    return len;
}

static ssize_t response_read(struct bt_conn *conn,
                             const struct bt_gatt_attr *attr, void *buf,
                             uint16_t len, uint16_t offset)
{
    return bt_gatt_attr_read(conn, attr, buf, len, offset, response, response_len);
}

BT_GATT_SERVICE_DEFINE(sosho_macro_svc,
    BT_GATT_PRIMARY_SERVICE(SOSHO_MACRO_SERVICE_UUID),
    BT_GATT_CHARACTERISTIC(SOSHO_MACRO_COMMAND_UUID,
        BT_GATT_CHRC_WRITE | BT_GATT_CHRC_WRITE_WITHOUT_RESP,
        BT_GATT_PERM_WRITE, NULL, command_write, NULL),
    BT_GATT_CHARACTERISTIC(SOSHO_MACRO_RESPONSE_UUID,
        BT_GATT_CHRC_READ | BT_GATT_CHRC_NOTIFY,
        BT_GATT_PERM_READ, response_read, NULL, NULL),
    BT_GATT_CCC(response_ccc, BT_GATT_PERM_READ | BT_GATT_PERM_WRITE)
);
