# SO-SHO firmware configuration

この非公開リポジトリは、公式SO-SHO templateを基にしたZMK firmware configurationです。日常構成は、右 = Central + trackball、左 = Peripheral + trackpadです。

## ビルド構成

- 日常構成: `sosho_R_trackball`（右Central） + `sosho_L_trackpad`（左Peripheral）
- `build.yaml` は右CentralのStudio USB endpointと `CONFIG_ZMK_STUDIO=y` を維持する
- settings reset buildと、他のtrackball / trackpad組み合わせもmatrixに残す
- 初回導入には右CentralのUF2をUSBで一度書き込む。以後のBLE Studio利用にはStudio対応build、BLE GATT transport、`studio_unlock` bindingが必要

## ポインティングデバイス設定

### Trackball

PMW3610と既存encoder moduleを維持し、`boards/shields/sosho/sosho.dtsi` の `SOSHO_TRACKBALL_CPI`、`SOSHO_TRACKBALL_XY_SCALE_NUM` / `SOSHO_TRACKBALL_XY_SCALE_DEN` を使う。値を変更したらfirmwareを再ビルドする。実機の体感値は未検証なので、Actions artifactを確認してから導入する。

### Trackpad

既存のGlidePoint / pointing input pathを維持し、`SOSHO_TRACKPAD_XY_SCALE_NUM` / `SOSHO_TRACKPAD_XY_SCALE_DEN` をmovement scaleとして使う。値を変更したら再ビルドが必要で、BLE Studioの実行時設定としては扱わない。Pinnacle sensitivityは現在1xで、upstream bindingの1x〜4xを再ビルドで選べる。

現行SO-SHO trackpad pathで安全に扱えるtap/click情報の範囲を確認するまでは、単一指のtap/clickだけを候補とする。finger count、raw contact、gesture phaseがdriverから確実に出ない場合、二指tap、二指double-tap、二指scrollを通常keyや時間判定で偽装しない。将来実装するなら、driverのcontact dataとinput processorのgesture state、テストを追加する必要がある。

## Studio / BLEの状態

BLE GATT Studio transportと `CONFIG_ZMK_BLE=y`、Studio unlock bindingを構成に含める。これはconfig上の準備であり、実機でのBLE接続、unlock後のkeymap read/write、Studio runtimeはこの作業では未検証である。USBは初回flashと復旧経路として残す。

## マクロの扱い

ZMK macro behaviorはcompile-timeのdevicetree定義であり、現行Studio keymap schemaにはマクロ定義の作成・編集・削除RPCがない。公式macro behaviorは `tap` / `press` / `release`、`wait-ms`、`tap-ms`、`pause-for-release`を扱う。したがってブラウザーeditorのマクロ名、target、`KEY|MODE|WAIT_MS`列はlocalStorageの設計データであり、このリポジトリのkeymapへ自動反映されるものではない。

ZMKのmacro sourceでwait/tapは `uint32_t`。UIの待ち時間は0〜4294967295の整数に正規化する。公式ドキュメントはBLEキー列で30ms以上のwait/tapを推奨し、Kconfigの既定値はwait 15ms、tap 30ms、behavior queue 64。実際のキー順序、長いmacroの収まり、layer / Bluetooth behaviorはcompile後の確認が必要である。

## 検証手順

1. GitHub Actionsで日常構成の右Central trackball / 左Peripheral trackpad artifactが成功することを確認する
2. 生成物のboard/shield、Studio USB endpoint、BLE設定、settings resetを確認する
3. 一度だけUSBで導入し、`studio_unlock`、BLE接続、Studio keymap RPCを実機で確認する
4. マクロをkeymapへcompile-timeで追加する場合は、別commitでbuildを通してからUSB/BLE運用へ進む
5. 不具合時は直前のUF2へ戻せるようにする

物理キーボードの操作、flash、再flashはこの編集作業では行っていない。
