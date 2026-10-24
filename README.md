# SO-SHO firmware configuration

The manufacturer circuit diagrams confirm the sensor wiring used by this
shield. In particular, trackpad selection is SPI-SS1/P1.05 and trackball
selection is SPI-SS2/P1.03. The trackpad SPI bus uses SCK/P1.13,
MOSI/P1.15, and MISO/P1.14; the PMW3610 trackball uses the official
three-wire/shared data configuration with both MOSI and MISO on P1.14.
Both use data-ready on P1.07. Source: <https://elekit.co.jp/support/faq/5255>.

このリポジトリは、公式SO-SHO templateを基にしたZMK firmware configurationです。日常構成は、右 = Central + trackball、左 = Peripheral + trackpadです。

## ビルド構成

- 日常構成: `sosho_R_trackball`（右Central） + `sosho_L_trackpad`（左Peripheral）
- 無操作時の自動スリープは左右すべて `CONFIG_ZMK_IDLE_SLEEP_TIMEOUT=3600000`（1時間）に設定する
- `build.yaml` は右CentralのStudio USB endpointと `CONFIG_ZMK_STUDIO=y` を維持する
- settings reset buildと、他のtrackball / trackpad組み合わせもmatrixに残す
- 最新成功ビルドの全バリエーションZIP: https://github.com/robo1129-ops/sosho-firmware/releases/download/latest/sosho-firmware-latest.zip
- ZIPを作るActions: https://github.com/robo1129-ops/sosho-firmware/actions/workflows/package-firmware.yml

ZIPには次の全パターンが含まれます。

- `sosho_R_trackball`: 右 / Central / トラックボール（PMW3610の共有SDIO=P1.14、SPI-SS2/P1.03）
- `sosho_L_trackpad`: 左 / Peripheral / トラックパッド
- `sosho_R_trackpad`: 右 / Central / トラックパッド
- `sosho_L_trackball`: 左 / Peripheral / トラックボール
- `settings_reset`: 設定消去用（通常の左右ファームとして書き込まない）
- 初回導入には右CentralのUF2をUSBで一度書き込む。以後のキーマップ編集も右CentralのUSB/COM経路だけを使う
- `sosho_R_trackball_touch` + `sosho_L_trackpad_touch` は、公式Zephyr Cirque Pinnacleドライバの一次タップを明示したタッチ操作プロファイル
- Windows接続向けに2M PHYを無効化し、必要な環境でLE passkey入力を有効化する。これはZMK公式のWindows接続トラブルシュートに沿った設定で、接続改善を保証するものではない

## ポインティングデバイス設定

### Trackball

PMW3610と既存encoder moduleを維持し、`boards/shields/sosho/sosho.dtsi` の `SOSHO_TRACKBALL_CPI`、`SOSHO_TRACKBALL_XY_SCALE_NUM` / `SOSHO_TRACKBALL_XY_SCALE_DEN` を使う。現在の標準ビルドはCPI 200（センサー側の下限）に加えて、トラックボールの移動量を `1/2` に減衰させる。値を変更したらfirmwareを再ビルドする。実機の体感値はActions artifactを確認してから導入する。

### Trackpad

既存のGlidePoint / pointing input pathを維持し、`SOSHO_TRACKPAD_XY_SCALE_NUM` / `SOSHO_TRACKPAD_XY_SCALE_DEN` をmovement scaleとして使う。現在は入力を減衰させない `1/1` と、Pinnacleの最大感度 `1x` にしている。値を変更したら再ビルドが必要で、Webアプリの実行時設定としては扱わない。

公式ZephyrのCirque Pinnacleドライバに `primary-tap-enable` を設定すると、単一指の接触が `INPUT_BTN_TOUCH` として出力され、ZMKの入力リスナーが左クリックとして扱う。これにより、一次タップ、接触を保持した移動によるドラッグ、2回のタップをホストOSがダブルクリックとして解釈する動作を、専用の独自プロトコルなしで利用できる。`sosho_L_trackpad` と `sosho_L_trackpad_touch` の両方に同じ経路を適用している。

Pinnacleは単一接触デバイスなので、指の本数、二指タップ、二指ダブルタップ、二指スクロール、ピンチはこのハードウェア／公式ドライバの入力だけでは実装できない。これらを時間判定で偽装する実装は行わない。多指ジェスチャーが必要な場合は、多点接触データを出せる別のタッチコントローラが必要になる。

### Tap dance

Web editorでは1回・2回・3回・ホールドのtap-dance定義を作成して物理キーへドラッグできます。日本語JISキーを含む通常キーとtap-dance定義はブラウザー側へ保存されます。tap-danceの実機キーマップへの反映は、現行のBLEマクロサービスとは別のZMK keymap bindingとして扱うため、定義をファームウェアへ組み込む場合はビルドが必要です。

## Studio / USBの状態

右Central側に公式のStudio USB UART transport、`CONFIG_ZMK_STUDIO=y`、Studio unlock bindingを構成する。PC側のStudio編集はBLE GATT transportを無効にし、Web SerialのUSB CDC-ACMだけを使用する。`CONFIG_ZMK_BLE=y` は左右分割通信と通常の無線キーボード動作のために残すが、PC側のキーマップ編集経路では使わない。左Peripheral側は左右通信専用で、PCのStudio終端ではない。

COMポートはUSB CDC/ACMのStudio UART transportであり、BLEをCOMポートとして公開する設定にはならない。Webアプリでは右CentralをUSB接続し、Web Serialから公式Studio protobuf RPCを実行する。

Web editorには公式Studio protobuf RPCのフレーム生成・応答解析・`get_device_info` / `get_lock_state` / `get_keymap` / `set_layer_binding` / `save_changes` / 再読み取りを実装済み。実機のUSB RPCはUF2導入後に確認する。ZMK公式ドキュメントも、USB利用時は `studio-rpc-usb-uart` snippetと `CONFIG_ZMK_STUDIO=y` を使い、分割キーボードでは中央側だけにUSB endpointを構成する手順を示している。

## マクロの扱い

公式ZMK Studio RPCはキーマップ編集用で、マクロ定義の作成・編集・削除RPCは提供していません。USB専用方針では、Web editorのマクロはJSON/localStorageへ保存し、実機へ書き込めるとは表示しません。実機で動くマクロにするには、ZMKの `behavior-macro` 定義をkeymapへ組み込み、ファームウェアを再ビルドしてUF2を書き込む必要があります。

保存形式は、Macro 0〜15の16スロット、名前、割り当て先、最大8ステップの `KEY|MODE|WAIT_MS` 列です。`MODE` は `tap` / `press` / `release`、待ち時間は0〜60000ミリ秒です。保存先はJSONとlocalStorageです。

PINを要求する特殊なホスト向けの予備として、Layer 3に数字入力を用意しています。通常レイヤーで右側のLayer 3キーを押している間、右手側の数字キーを使い、右下のキーでENTERを送信できます。通常のWindowsペアリングではPIN入力を使わない構成です。

Windowsで「接続済みだがキーやポインティング入力が届かない」状態を避けるため、中央側では `CONFIG_BT_GATT_ENFORCE_SUBSCRIPTION=n` を設定しています。また、split中央のBattery Level代理通知・取得を無効にし、左右入力のposition-state GATT探索を優先します。ZMKのsplit関連既知事例では、Battery Level取得を有効にした探索中にPeripheralのposition-state characteristicが見つからず、左右入力が届かなくなる症状が報告されています。

Web editorでは、マクロを編集してJSONへ保存・読み込みできます。MIDIは仕様・UI・プロトコルに含めません。

## 検証手順

1. GitHub Actionsで日常構成の右Central trackball / 左Peripheral trackpad artifactが成功することを確認する
2. 生成物のboard/shield、Studio USB endpoint、BLE設定、settings resetを確認する
3. 一度だけUSBで導入し、右Centralがホストに接続できること、左Peripheralのキーが右Central経由で入力されることを実機で確認する
4. 右CentralのUSBから公式Studio RPCのkeymap read/write/re-readを実機で確認する
5. touchプロファイルで一次タップ、長押し移動のドラッグ、ホスト側ダブルクリックを確認する
6. 不具合時は直前のUF2へ戻せるようにする

このリポジトリのソース変更とActionsビルド以外の物理キーボード操作、flash、再flashは行っていない。実機でのUSB RPCとtouchプロファイルの操作確認は、生成UF2導入後にユーザー側で確認する必要がある。
