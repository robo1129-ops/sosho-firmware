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
- 無操作時の自動スリープは、5分・15分・30分・1時間・スリープなしの5種類をビルドする。各種類はZIP内の `sleep-5m/`、`sleep-15m/`、`sleep-30m/`、`sleep-60m/`、`sleep-off/` フォルダに分けて収録する
- `build.yaml` は右CentralのStudio USB endpointと `CONFIG_ZMK_STUDIO=y` を維持する
- settings reset buildと、他のtrackball / trackpad組み合わせもmatrixに残す
- 最新成功ビルドの全バリエーションZIP: https://github.com/robo1129-ops/sosho-firmware/releases/download/latest/sosho-firmware-latest.zip
- 全スリープ設定・全モジュール構成のローカル検証済みZIP: https://github.com/robo1129-ops/sosho-firmware/raw/main/downloads/sosho-firmware-latest-local.zip
- ZIPを作るActions: https://github.com/robo1129-ops/sosho-firmware/actions/workflows/package-firmware.yml

## フラッシュ容量とマクロ保存容量

2026-10-24の最新成功ビルド（commit `dc91c6b`）のUF2を展開して、UF2の実データ領域を測定した。XIAO nRF52840の標準UF2レイアウトでは、アプリケーション領域は `0x27000` から `0xEC000` までの `0xC5000` bytes（806,912 bytes / 788 KiB）で、32 KiBの設定保存領域とUF2ブート領域は別に確保されている。

最も大きい中央側イメージは `sosho_R_trackball.uf2` で、実データは 290,048 bytes（283.25 KiB、35.95%）だった。したがって、アプリケーション領域には 516,864 bytes（504.75 KiB、64.05%）の余裕がある。左側の `sosho_L_trackpad.uf2` は 211,712 bytes（206.75 KiB）で、余裕は 595,200 bytes（581.25 KiB）ある。UF2ファイルの見かけのサイズにはUF2レコードの管理領域が含まれるため、容量判断には実データ領域を使っている。

マクロは16スロット、1スロット最大224 bytes、最大8ステップで、マクロ本体の最大合計は 3,584 bytes（16 × 224）である。これはアプリケーション領域ではなく、32 KiBの設定保存領域へNVSとして保存する。NVSのレコード管理領域やキーマップなど他の設定も同じ保存領域を使うため、3,584 bytes全てがそのまま空き容量になるわけではないが、現在のマクロ上限は設定保存領域に対して十分小さい。実機の設定領域の残量そのものは、NVSの既存データ量を読み取らない限りビルド成果物だけからは確定できない。

ZIPには次の全パターンが各スリープ時間フォルダに含まれます。

- `sleep-5m/`: 5分で自動スリープ
- `sleep-15m/`: 15分で自動スリープ
- `sleep-30m/`: 30分で自動スリープ
- `sleep-60m/`: 1時間で自動スリープ
- `sleep-off/`: 自動スリープなし

各フォルダには、以下の右/左モジュール組み合わせが同じ命名規則で入ります。

- `sosho_R_trackball`: 右 / Central / トラックボール（PMW3610の共有SDIO=P1.14、SPI-SS2/P1.03）
- `sosho_L_trackpad`: 左 / Peripheral / トラックパッド
- `sosho_R_trackpad`: 右 / Central / トラックパッド
- `sosho_L_trackball`: 左 / Peripheral / トラックボール
- `sosho_R_none` + `sosho_L_none`: 左右ともブランクのダミーモジュール。行列、エンコーダー、分割通信だけを残し、ローカルのポインティングセンサーは有効にしない
- `sosho_R_none` は中央側の分割入力リスナーを残すため、実際のセンサーを持つPeripheralと組み合わせられる。`sosho_L_none` はセンサーなしのPeripheralとして使う
- `settings_reset`: 設定消去用（通常の左右ファームとして書き込まない）
- 初回導入には右CentralのUF2をUSBで一度書き込む。以後のキーマップ編集も右CentralのUSB/COM経路だけを使う
- Windows接続向けに2M PHYを無効化し、必要な環境でLE passkey入力を有効化する。これはZMK公式のWindows接続トラブルシュートに沿った設定で、接続改善を保証するものではない

左右のモジュールを独立して選ぶ場合は、中央側を `sosho_R_trackball` / `sosho_R_trackpad` / `sosho_R_none` から、Peripheral側を `sosho_L_trackpad` / `sosho_L_trackball` / `sosho_L_none` から選ぶ。WebアプリのNone選択はこの組み合わせに対応する。左右を逆にした場合は、中央側を必ず右手でUSB接続する構成にする。

## ポインティングデバイス設定

Webエディターの自動スリープ設定は、分指定またはオフを選べます。これはキーマップRPCで即時変更する値ではなく、ファームウェアのビルド時設定です。ZIPでは希望する `sleep-*` フォルダから左右のUF2を同じ組み合わせで書き込んでください。

### Trackball

PMW3610と既存encoder moduleを維持し、`boards/shields/sosho/sosho.dtsi` の `SOSHO_TRACKBALL_CPI`、`SOSHO_TRACKBALL_XY_SCALE_NUM` / `SOSHO_TRACKBALL_XY_SCALE_DEN` を使う。現在の標準ビルドはCPI 200（センサー側の下限）に加えて、水平・垂直軸を反転し、トラックボールの移動量を `5/6`（約83.3%）に減衰させる。スクロールモードは通常移動とは別に、水平軸を維持しつつ垂直方向を反転し、マッピング後のホイール出力を `7/20`（35%）に設定している。値を変更したらfirmwareを再ビルドする。実機の体感値はActions artifactを確認してから導入する。

レイヤー1の右手中段中央（従来MB3の位置）は、レイヤー4を押下中だけ有効にする `MO(4)` に変更している。レイヤー4ではトラックボールのX/Y入力をスクロール入力へ変換するため、キーを押している間だけスクロールモードになる。ZMK Studioで既存の保存済みキーマップを使用している場合は、同じキーに `MO(4)` を割り当てるか、Restore Stock Settings後に反映する。

レイヤー4のトラックボールスクロールには慣性スクロールを実装している。入力を止めた後に約45ms待ってから、直前の速度を16ms間隔で7/8ずつ減衰させるため、自然なスクロールの余韻が出る。スクロール量はライブ入力・慣性入力とも基準値の35%で、前回の70%設定の半分にしている。垂直方向は通常のスクロールと逆になる。左右・上下の軸固定はそのまま維持し、通常のポインター移動やトラックパッドには適用しない。

ロータリーエンコーダーは押下入力を持たない。標準の `config/sosho.keymap` では、左エンコーダーの反時計回りが左右スクロール左、時計回りが左右スクロール右、右エンコーダーの反時計回りが上下スクロール下、時計回りが上下スクロール上である。Webアプリのエンコーダー設定も同じ順序で表示する。ZMK Studioの標準キーマップRPCはキー割り当てを扱うが、センサーの `sensor-bindings` を実行時に変更しないため、エンコーダー動作をファームウェアへ反映するには再ビルドが必要になる。

Webアプリでは、左右それぞれの反時計回り・時計回りを通常キーと同じアクションパレットから編集できる。エンコーダーは押下できないため、ホールド／タップ設定は表示せず、回転方向への単一アクションだけを保存する。設定はJSONへ保存され、通常キーのRPC書き込みに混ぜない。現在のファームウェアで実機へ反映できるのは、ビルド時の `sensor-bindings` に定義された動作であり、任意のエンコーダー割り当てを実機へ反映するには、選択内容を取り込んだファームウェアの再ビルドが必要になる。

### Trackpad

既存のGlidePoint / pointing input pathを維持し、`SOSHO_TRACKPAD_XY_SCALE_NUM` / `SOSHO_TRACKPAD_XY_SCALE_DEN` をmovement scaleとして使う。無反応を起こした絶対座標の独自変換は、通常の復旧ビルドでは使用しない。公式Zephyr Pinnacleドライバのrelative modeから出る `INPUT_REL_X` / `INPUT_REL_Y` と `INPUT_BTN_TOUCH` を、分割入力リスナーでそのまま処理するため、接触イベントのタイミングに依存して移動を全て捨てることがない。Pinnacleの感度は `1x`、中央側の移動倍率は `1:1` に戻した。SO-SHOの実機向きに合わせてX軸だけを反転し、Y軸は標準方向にする。値を変更したら再ビルドが必要で、Webアプリの実行時設定としては扱わない。

短いタップは左クリック、500ms以上の接触は右クリックとして扱い、長押し後の移動は右ボタンを押したままのドラッグとして出力する。接触後の移動量が24カウント以内なら、指の小さなずれを許容してタップを維持する。24カウントを超える移動はドラッグとして扱い、誤クリックを出さない。短いタップを2回行った場合は、2回の標準左クリックを送るため、ホストOSがダブルクリックとして解釈できる。これは公式relative modeの `BTN_TOUCH` を中央側の長押しプロセッサへ渡す構成で、USB/BLEのホスト経路に依存しない。

以前のabsolute modeでは、接触開始／終了のX/Y/Zフレームを独自に組み替えて外周スクロールを実装していたが、個体や分割転送のイベント順序が異なるとポインター移動まで抑止される問題があった。そのため今回の復旧版では外周をなぞる自動スクロールを一旦無効化し、標準のポインター移動・クリック・右ドラッグを優先する。スクロールは従来どおり右トラックボールのスクロールレイヤーを使用する。外周スクロールを再導入する場合は、relative pathを壊さない別入力経路として検証する必要がある。

長押しの判定値は `boards/shields/sosho/sosho.dtsi` の `long-press-ms = <500>` で管理する。入力イベントをUSB/BLEのどちらかに依存して処理せず、ZMKの中央側入力リスナーの前段で右ボタンへ変換するため、両方のホスト接続方式で同じ動作になる。

タッチパッド側の外周スクロールは、この復旧版では無効です。現在の公式relative modeには接触座標がなく、外周判定を安全に行えないためです。過去に絶対座標を独自に再解釈した際、接触位置の変化をポインターの瞬間移動や無反応に変えていたため、クリックと移動の安定性を優先しています。スクロールは従来どおり右トラックボールのスクロールレイヤーを使用します。

Pinnacleは単一接触デバイスなので、指の本数、二指タップ、二指ダブルタップ、二指スクロール、ピンチはこのハードウェア／公式ドライバの入力だけでは実装できない。今回のダブルクリックは、2回の短いタップを標準の左クリック2回としてホストOSへ渡す方式である。多指ジェスチャーが必要な場合は、多点接触データを出せる別のタッチコントローラが必要になる。

### Tap dance

Web editorでは1回・2回・3回・ホールドのtap-dance定義を作成して物理キーへドラッグできます。日本語JISキーを含む通常キーとtap-dance定義はブラウザー側へ保存されます。tap-danceの実機キーマップへの反映は、現行のBLEマクロサービスとは別のZMK keymap bindingとして扱うため、定義をファームウェアへ組み込む場合はビルドが必要です。

## Studio / USBの状態

右Central側に公式のStudio USB UART transport、公式Studio BLE GATT transport、`CONFIG_ZMK_STUDIO=y`、Studio unlock bindingを構成する。USBは右CentralのCDC-ACM、BLEは公式GATTサービスを使う。左Peripheral側は左右通信専用で、PCのStudio終端ではない。WindowsのWeb Bluetoothは制約があるため、BLE編集は対応ブラウザー／ネイティブStudioで行い、WindowsではUSBを推奨する。

COMポートはUSB CDC/ACMのStudio UART transportであり、BLEをCOMポートとして公開する設定にはならない。Webアプリは右CentralのUSBと、対応環境では公式Studio BLE GATTの両方から同じprotobuf RPCを実行する。USB接続時に左Peripheralを選択した場合は編集対象外として警告する。

Web editorには公式Studio protobuf RPCのフレーム生成・応答解析・`get_device_info` / `get_lock_state` / `get_keymap` / `set_layer_binding` / `save_changes` / 再読み取りを実装済み。実機のUSB RPCはUF2導入後に確認する。ZMK公式ドキュメントも、USB利用時は `studio-rpc-usb-uart` snippetと `CONFIG_ZMK_STUDIO=y` を使い、分割キーボードでは中央側だけにUSB endpointを構成する手順を示している。

## マクロの扱い

公式ZMK Studio RPCはキーマップ編集用で、マクロ定義の作成・編集・削除RPCは提供していません。そのため本ファームウェアでは、公式RPCとは別に暗号化済みSO-SHOマクロBLE GATTサービスをCentralへ追加し、Web editorからMacro 0〜15を保存・削除・実行できるようにします。USB接続時は公式RPCの仕様上、マクロ送信ではなくJSON保存までです。

保存形式は、Macro 0〜15の16スロット、名前、割り当て先、最大8ステップの `KEY|MODE|WAIT_MS` 列です。`MODE` は `tap` / `press` / `release`、待ち時間は0〜60000ミリ秒です。保存先はJSON、localStorage、対応BLE時のZMK settingsです。

PINを要求する特殊なホスト向けの予備として、Layer 3に数字入力を用意しています。通常レイヤーで右側のLayer 3キーを押している間、右手側の数字キーを使い、右下のキーでENTERを送信できます。通常のWindowsペアリングではPIN入力を使わない構成です。

Windowsで「接続済みだがキーやポインティング入力が届かない」状態を避けるため、中央側では `CONFIG_BT_GATT_ENFORCE_SUBSCRIPTION=n` を設定しています。また、split中央のBattery Level代理通知・取得を無効にし、左右入力のposition-state GATT探索を優先します。ZMKのsplit関連既知事例では、Battery Level取得を有効にした探索中にPeripheralのposition-state characteristicが見つからず、左右入力が届かなくなる症状が報告されています。

Web editorでは、マクロを編集してJSONへ保存・読み込みできます。MIDIは仕様・UI・プロトコルに含めません。

## 検証手順

1. GitHub Actionsで日常構成の右Central trackball / 左Peripheral trackpad artifactが成功することを確認する
2. 生成物のboard/shield、Studio USB endpoint、BLE設定、settings resetを確認する
3. 一度だけUSBで導入し、右Centralがホストに接続できること、左Peripheralのキーが右Central経由で入力されることを実機で確認する
4. 右CentralのUSBから公式Studio RPCのkeymap read/write/re-readを実機で確認する
5. 標準トラックパッドプロファイルでポインター移動、タップの左クリック、2回タップのホスト側ダブルクリック、500ms長押しの右クリック、長押し移動の右ドラッグを確認する。スクロールは右トラックボールのスクロールレイヤーで確認する
6. 不具合時は直前のUF2へ戻せるようにする

このリポジトリのソース変更とActionsビルド以外の物理キーボード操作、flash、再flashは行っていない。実機でのUSB RPC、BLE Studio、トラックパッド操作確認は、生成UF2導入後にユーザー側で確認する必要がある。
