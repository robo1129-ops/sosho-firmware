# SO-SHO firmware configuration

The manufacturer circuit diagrams confirm the sensor wiring used by this
shield. In particular, trackpad selection is SPI-SS1/P1.05 and trackball
selection is SPI-SS2/P1.03. The trackpad SPI bus uses SCK/P1.13,
MOSI/P1.15, and MISO/P1.14; the PMW3610 trackball uses the official
three-wire/shared data configuration with both MOSI and MISO on P1.14.
Both use data-ready on P1.07. Source: <https://elekit.co.jp/support/faq/5255>.

このリポジトリは、公式SO-SHO templateを基にしたZMK firmware configurationです。日常構成は、右 = Central + trackball、左 = Peripheral + trackpadです。

## Webエディターとの対応契約

現在のSO-SHO Webエディターは、右Centralの公式ZMK Studio RPCで実機へ保存できる設定だけを編集画面に表示する。通常キー、修飾キー、JISキー、レイヤー操作、ファームウェアのメタデータに存在するホールド／タップ、マウス・ホイール・Bluetooth・出力操作、CPI・トラックパッド倍率・スクロール倍率の増減が対象である。SO-SHO拡張RPC対応ファームウェアでは、マクロと実行時スリープ時間も同じUSBまたはBLEのStudio RPC経路で保存できる。保存後は実機から再読み取りし、送信内容と一致することを確認する。

初期CPI、初期センサー倍率、軸反転、軸固定、センサー配置、ロータリーエンコーダーのsensor-bindingsは、公式ZMK StudioのキーマップRPCだけでは変更できないため、ビルド構成で選択する。スリープ時間とマクロはSO-SHO拡張RPC対応ファームウェアでのみ実機保存できる。設定ファイルのJSONは公式キーマップRPCで扱えるキーマップとデフォルトキーマップを対象とし、マクロとスリープは実機から読み書きできる拡張形式として別途保存する。実機へ書き込めない値を成功したようには扱わない。

## ビルド構成

- 日常構成: `sosho_R_trackball`（右Central） + `sosho_L_trackpad`（左Peripheral）
- 右側にトラックパッドを配置する構成では、右モジュールの実装向きに合わせてX/Yの両軸を反転する
- 無操作時の自動スリープは、5分・15分・30分・1時間・スリープなしの5種類をビルドする。各種類はZIP内の `sleep-5m/`、`sleep-15m/`、`sleep-30m/`、`sleep-60m/`、`sleep-off/` フォルダに分けて収録する
- `build.yaml` は右CentralのStudio USB endpointと `CONFIG_ZMK_STUDIO=y` を維持する
- settings reset buildと、他のtrackball / trackpad組み合わせもmatrixに残す
- 最新成功ビルドの全バリエーションZIP: https://github.com/robo1129-ops/sosho-firmware/releases/download/latest/sosho-firmware-latest.zip
- 全スリープ設定・全モジュール構成のZIP（Actionsの最新成功ビルド）: https://github.com/robo1129-ops/sosho-firmware/releases/download/latest/sosho-firmware-latest.zip
- ZIPを作るActions: https://github.com/robo1129-ops/sosho-firmware/actions/workflows/package-firmware.yml

## フラッシュ容量とマクロ保存容量

2026-10-24の最新成功ビルド（commit `dc91c6b`）のUF2を展開して、UF2の実データ領域を測定した。XIAO nRF52840の標準UF2レイアウトでは、アプリケーション領域は `0x27000` から `0xEC000` までの `0xC5000` bytes（806,912 bytes / 788 KiB）で、32 KiBの設定保存領域とUF2ブート領域は別に確保されている。

最も大きい中央側イメージは `sosho_R_trackball.uf2` で、実データは 290,048 bytes（283.25 KiB、35.95%）だった。したがって、アプリケーション領域には 516,864 bytes（504.75 KiB、64.05%）の余裕がある。左側の `sosho_L_trackpad.uf2` は 211,712 bytes（206.75 KiB）で、余裕は 595,200 bytes（581.25 KiB）ある。UF2ファイルの見かけのサイズにはUF2レコードの管理領域が含まれるため、容量判断には実データ領域を使っている。

マクロは16スロット、1スロット最大512 bytes、最大64ステップで、マクロ本体の最大合計は 8,192 bytes（16 × 512）である。これはアプリケーション領域ではなく、32 KiBの設定保存領域へNVSとして保存する。NVSのレコード管理領域やキーマップなど他の設定も同じ保存領域を使うため、8,192 bytes全てがそのまま空き容量になるわけではないが、現在のマクロ上限は設定保存領域に対して十分小さい。実機の設定領域の残量そのものは、NVSの既存データ量を読み取らない限りビルド成果物だけからは確定できない。

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

自動スリープは、SO-SHO拡張RPC対応ファームウェアでは実行時にオフまたは分単位で変更できる。未対応の旧ファームウェアや拡張を無効にした構成では、従来どおりビルド時設定となるため、ZIP内の希望する `sleep-*` フォルダから左右のUF2を書き込む。Webエディターは、接続後に実機が `runtime_control` を返した場合だけ保存操作を有効にする。

### Trackball

PMW3610と既存encoder moduleを維持し、`boards/shields/sosho/sosho.dtsi` の `SOSHO_TRACKBALL_CPI`、`SOSHO_TRACKBALL_XY_SCALE_NUM` / `SOSHO_TRACKBALL_XY_SCALE_DEN` を使う。現在の標準ビルドはCPI 200（センサー側の下限）に加えて、水平・垂直軸を反転し、トラックボールの移動量を `5/6`（約83.3%）に減衰させる。スクロールモードは通常移動とは別に、水平軸を維持しつつ垂直方向を反転し、マッピング後のホイール出力を `7/20`（35%）に設定している。値を変更したらfirmwareを再ビルドする。実機の体感値はActions artifactを確認してから導入する。

レイヤー1の右手中段中央（従来MB3の位置）は、レイヤー4を押下中だけ有効にする `MO(4)` に変更している。レイヤー4ではトラックボールのX/Y入力をスクロール入力へ変換するため、キーを押している間だけスクロールモードになる。ZMK Studioで既存の保存済みキーマップを使用している場合は、同じキーに `MO(4)` を割り当てるか、Restore Stock Settings後に反映する。

レイヤー4のトラックボールスクロールには慣性スクロールを実装している。入力を止めた後に約45ms待ってから、直前の速度を16ms間隔で7/8ずつ減衰させるため、自然なスクロールの余韻が出る。スクロール量はライブ入力・慣性入力とも基準値の35%で、前回の70%設定の半分にしている。垂直方向は通常のスクロールと逆になる。左右・上下の軸固定はそのまま維持し、通常のポインター移動やトラックパッドには適用しない。

ロータリーエンコーダーは押下入力を持たない。標準の `config/sosho.keymap` では、左エンコーダーの反時計回りが左右スクロール左、時計回りが左右スクロール右、右エンコーダーの反時計回りが上下スクロール下、時計回りが上下スクロール上である。Webアプリのエンコーダー設定も同じ順序で表示する。エンコーダーはファームウェア内の `sosho,behavior-encoder-mouse-scroll` で、角度形式とティック形式の両方のセンサー通知を受け付け、1回の回転を1回のスクロール通知として送信するため、旧来の押下・解放方式やティック形式の読み落としを避けられる。ZMK Studioの標準キーマップRPCはキー割り当てを扱うが、センサーの `sensor-bindings` を実行時に変更しないため、エンコーダー動作をファームウェアへ反映するには再ビルドが必要になる。

Webアプリでは、ロータリーエンコーダーの押下なしという実機形状を表示上も維持し、ホールド／タップ設定や実機へ書き込めない回転割り当ての編集を表示しない。現在のファームウェアで実機へ反映されるのは、ビルド時の `sensor-bindings` に定義された動作であり、回転方向を変更するには選択内容を取り込んだファームウェアの再ビルドが必要になる。

### Trackpad

既存のGlidePoint / pointing input pathを維持し、`SOSHO_TRACKPAD_XY_SCALE_NUM` / `SOSHO_TRACKPAD_XY_SCALE_DEN` をmovement scaleとして使う。無反応や瞬間移動を起こした絶対座標の独自変換は全構成から外し、公式Zephyr Pinnacleドライバのrelative modeを使う。relative modeから出る `INPUT_REL_X` / `INPUT_REL_Y` と `INPUT_BTN_TOUCH` をそのままZMKの入力リスナーへ渡すため、接触位置を変えても座標ジャンプを生成しない。Pinnacleの感度は `1x`、移動倍率は `1:1` である。

短いタップは公式ドライバのprimary tapによる左クリックとして扱う。短いタップを2回行った場合は、2回の標準左クリックを送り、ホストOSがダブルクリックとして解釈できる。今回の安定版では右クリック判定、長押し右ドラッグ、トラックパッド外周スクロールは実装しない。相対モードでは接触座標が得られないため、外周判定を安全に行えないためである。スクロールは従来どおり右トラックボールのスクロールレイヤーを使用する。

左側トラックパッドはSO-SHO左側の実装向きに合わせてX軸を反転する。右側トラックパッド構成（`sosho_R_trackpad`）は、右側に配置したときの物理向きに合わせてX/Yの両軸を反転する。これにより、右側配置時の上下左右反転を構成ごとに補正する。値を変更したら再ビルドが必要で、Webアプリの実行時設定としては扱わない。

Pinnacleは単一接触デバイスなので、指の本数、二指タップ、二指ダブルタップ、二指スクロール、ピンチはこのハードウェア／公式ドライバの入力だけでは実装できない。今回のダブルクリックは、2回の短いタップを標準の左クリック2回としてホストOSへ渡す方式である。多指ジェスチャーが必要な場合は、多点接触データを出せる別のタッチコントローラが必要になる。

### Tap dance

Tap danceはファームウェアのkeymap bindingとしてビルド時に定義する機能であり、公式ZMK Studioの標準キーマップRPCだけでは新しい定義を作成できない。現在のWebエディターでは、実機へ保存できないtap-dance定義やドラッグ操作を編集画面に表示しない。必要な定義はkeymapへ追加してファームウェアを再ビルドする。

### 同じキーのタップ後ホールド

SO-SHO拡張ファームウェアでは、タップ／ホールドキーを一度タップして離した直後、同じキーをもう一度押して押し続けると、
タップ側のキーコードを押し続ける動作になります。例えば、Backspaceのタップ／Layer 3のホールド設定では、
「Backspaceをタップして離す → 同じキーを押し続ける」とBackspaceの連続入力になります。これは通常のキースイッチの
押しっぱなしに近い動作です。一方、直前のタップがない状態で最初から長押しした場合は、従来どおり設定されたホールド側
（Shift、Alt、レイヤー移動など）が選択されます。別のキーの動作や通常キーの入力は変更しません。

## Studio / USBの状態

右Central側に公式のStudio USB UART transport、`CONFIG_ZMK_STUDIO=y`、Studio unlock bindingを構成する。USBは右CentralのCDC-ACMを使い、左Peripheral側は左右通信専用でPCのStudio終端ではない。BLE transportを有効にした右Centralでは、Webエディターが公式のZMK Studio GATTサービスを使い、USBと同じRPCを送受信する。USB／BLEのどちらでも保存結果を実機から再読み取りして一致確認する。

COMポートはUSB CDC/ACMのStudio UART transportであり、BLEをCOMポートとして公開する設定にはならない。Webエディターは右CentralのUSB/COMへ接続し、左Peripheralを編集対象として扱わない。右Central以外を接続した場合は保存を実行せず、対象外であることを表示する。

Web editorには公式Studio protobuf RPCのフレーム生成・応答解析・`get_device_info` / `get_lock_state` / `get_keymap` / `set_layer_binding` / `save_changes` / 再読み取りに加え、SO-SHO拡張の `macro` / `sosho` サブシステムを実装済み。実機のUSB／BLE RPCは、拡張パッチを適用したUF2導入後に確認する。ZMK公式ドキュメントも、USB利用時は `studio-rpc-usb-uart` snippetと `CONFIG_ZMK_STUDIO=y` を使い、分割キーボードでは中央側だけにUSB endpointを構成する手順を示している。

## マクロと実行時スリープ

公式ZMK Studio RPCにはマクロ定義の標準サブシステムがないため、SO-SHO用に `zmk.macro` と `zmk.sosho` の拡張サブシステムを追加している。マクロは16スロット、1スロット512 bytes、最大64ステップで、各ステップはキー、tap／press／release、待ち時間（初期値1 ms）を持つ。Webエディターのドラッグ編集後、USBまたはBLEのStudio RPCで保存し、全スロットを再読み取りして一致を確認する。

スリープ時間は `zmk.sosho` の `get_sleep` / `set_sleep_timeout_ms` で、オフまたは1分単位の1〜1440分を受け付ける。保存値はZephyr settingsへ保存し、次回起動後も読み込む。ZMK core側には、ビルド時の `CONFIG_ZMK_IDLE_SLEEP_TIMEOUT` をデフォルトにしつつ、保存値が0ならスリープを無効化し、正の保存値ならその値を使うフックを適用している。

この拡張はSO-SHO用パッチを適用したファームウェア専用で、公式ZMKの未改造バイナリでは利用できない。Web側は拡張RPCの応答がない場合、マクロ／実行時スリープの保存を成功扱いにしない。

PINを要求する特殊なホスト向けの予備として、Layer 3に数字入力を用意しています。通常レイヤーで右側のLayer 3キーを押している間、右手側の数字キーを使い、右下のキーでENTERを送信できます。通常のWindowsペアリングではPIN入力を使わない構成です。

Windowsで「接続済みだがキーやポインティング入力が届かない」状態を避けるため、中央側では `CONFIG_BT_GATT_ENFORCE_SUBSCRIPTION=n` を設定しています。また、split中央のBattery Level代理通知・取得を無効にし、左右入力のposition-state GATT探索を優先します。ZMKのsplit関連既知事例では、Battery Level取得を有効にした探索中にPeripheralのposition-state characteristicが見つからず、左右入力が届かなくなる症状が報告されています。

JSONのダウンロード／読み込みは、実機の公式RPCが扱えるキーマップとデフォルトキーマップだけを対象とする。MIDI、ブラウザー専用マクロ、実機へ反映できないセンサー設定はJSONへ含めない。

## 検証手順

1. GitHub Actionsで日常構成の右Central trackball / 左Peripheral trackpad artifactが成功することを確認する
2. 生成物のboard/shield、Studio USB endpoint、BLE設定、settings resetを確認する
3. 一度だけUSBで導入し、右Centralがホストに接続できること、左Peripheralのキーが右Central経由で入力されることを実機で確認する
4. 右CentralのUSBから公式Studio RPCのkeymap read/write/re-readを実機で確認する。保存操作は `save_changes` の応答を受けてから再読み取りし、一致しない場合は成功扱いにしない
5. 標準トラックパッドプロファイルでポインター移動、タップの左クリック、2回タップのホスト側ダブルクリックを確認する。右クリックと外周スクロールは今回の安定版では対象外。スクロールは右トラックボールのスクロールレイヤーで確認する
6. 不具合時は直前のUF2へ戻せるようにする

このリポジトリのソース変更とローカル／Actionsビルド以外の物理キーボード操作、flash、再flashは行っていない。現時点で確認済みなのは、設定保存用NVSを有効にした構成でビルドが通ること、Web側のRPC変換と再読み取りテストが通ることまでである。実機での保存成功は、新しい右Central UF2を書き込んだ後に、右CentralをUSB接続して読み取り→変更→保存→再読み取りの順で確認する必要がある。
