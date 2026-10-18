# SO-SHO firmware configuration

このリポジトリは、公式SO-SHO templateを基にしたZMK firmware configurationです。日常構成は、右 = Central + trackball、左 = Peripheral + trackpadです。

## ビルド構成

- 日常構成: `sosho_R_trackball`（右Central） + `sosho_L_trackpad`（左Peripheral）
- `build.yaml` は右CentralのStudio USB endpointと `CONFIG_ZMK_STUDIO=y` を維持する
- settings reset buildと、他のtrackball / trackpad組み合わせもmatrixに残す
- 最新成功ビルドの全バリエーションZIP: https://github.com/robo1129-ops/sosho-firmware/releases/download/latest/sosho-firmware-latest.zip
- ZIPを作るActions: https://github.com/robo1129-ops/sosho-firmware/actions/workflows/package-firmware.yml

ZIPには次の全パターンが含まれます。

- `sosho_R_trackball`: 右 / Central / トラックボール
- `sosho_L_trackpad`: 左 / Peripheral / トラックパッド
- `sosho_R_trackpad`: 右 / Central / トラックパッド
- `sosho_L_trackball`: 左 / Peripheral / トラックボール
- `settings_reset`: 設定消去用（通常の左右ファームとして書き込まない）
- 初回導入には右CentralのUF2をUSBで一度書き込む。以後のBLE Studio利用にはStudio対応build、BLE GATT transport、`studio_unlock` bindingが必要

## ポインティングデバイス設定

### Trackball

PMW3610と既存encoder moduleを維持し、`boards/shields/sosho/sosho.dtsi` の `SOSHO_TRACKBALL_CPI`、`SOSHO_TRACKBALL_XY_SCALE_NUM` / `SOSHO_TRACKBALL_XY_SCALE_DEN` を使う。値を変更したらfirmwareを再ビルドする。実機の体感値は未検証なので、Actions artifactを確認してから導入する。

### Trackpad

既存のGlidePoint / pointing input pathを維持し、`SOSHO_TRACKPAD_XY_SCALE_NUM` / `SOSHO_TRACKPAD_XY_SCALE_DEN` をmovement scaleとして使う。値を変更したら再ビルドが必要で、BLE Studioの実行時設定としては扱わない。Pinnacle sensitivityは現在1xで、upstream bindingの1x〜4xを再ビルドで選べる。

現行SO-SHO trackpad pathで安全に扱えるtap/click情報の範囲を確認するまでは、単一指のtap/clickだけを候補とする。finger count、raw contact、gesture phaseがdriverから確実に出ない場合、二指tap、二指double-tap、二指scrollを通常keyや時間判定で偽装しない。将来実装するなら、driverのcontact dataとinput processorのgesture state、テストを追加する必要がある。

### Tap dance

全4モジュール構成でZMK標準のtap-dance behaviorを有効にしています。Web editorでは1回・2回・3回・ホールドの定義を作成して物理キーへドラッグできます。日本語JISキーを含む通常キーとtap-dance定義はブラウザー側へ保存されます。tap-danceの実機キーマップへの反映は、現行のBLEマクロサービスとは別のZMK keymap bindingとして扱うため、定義をファームウェアへ組み込む場合はビルドが必要です。

## Studio / BLEの状態

BLE GATT Studio transportと `CONFIG_ZMK_BLE=y`、Studio unlock bindingを構成に含める。これはconfig上の準備であり、実機でのBLE接続、unlock後のkeymap read/write、Studio runtimeはこの作業では未検証である。USBは初回flashと復旧経路として残す。

## マクロの扱い

公式ZMK Studio RPCにはマクロ定義の作成・編集・削除機能がないため、SO-SHO専用のBLE GATTサービスを追加しています。初回だけこのBLE対応版ファームウェアをUSBで書き込みます。その後は公開Web editorから暗号化されたBLE接続でマクロを保存・読み戻し・実行でき、マクロ編集のたびに再ビルドする必要はありません。

保存形式は、マクロID、名前、割り当て先、最大8ステップの `KEY|MODE|WAIT_MS` 列です。`MODE` は `tap` / `press` / `release`、待ち時間は0〜60000ミリ秒です。保存先はZephyr Settingsで、電源再投入後も保持されます。BLEサービスはバージョン、長さ、CRC、応答ステータス、暗号化リンク、最大サイズを検証します。

Web editorでは、BLE接続後に「BLEへ保存」「BLEから読み込む」「実行」を使います。キーボード側のBluetoothペアリングと暗号化接続が必要です。MIDIは仕様・UI・プロトコルに含めません。

## 検証手順

1. GitHub Actionsで日常構成の右Central trackball / 左Peripheral trackpad artifactが成功することを確認する
2. 生成物のboard/shield、Studio USB endpoint、BLE設定、settings resetを確認する
3. 一度だけUSBで導入し、Bluetoothペアリング、暗号化接続、BLEマクロの保存・読み戻し・実行を実機で確認する
4. マクロ編集のたびに再ビルドせず、Web editorから保存できることを確認する
5. 不具合時は直前のUF2へ戻せるようにする

このリポジトリのソース変更とActionsビルド以外の物理キーボード操作、flash、再flash、Bluetoothペアリングは行っていない。実機でのBLE送受信は、初回ファームウェア導入後にユーザー側で確認する必要がある。
