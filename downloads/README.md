# SO-SHO trackpad gesture build

実行時スリープ対応・全モジュール構成をまとめた最新ZIPは、GitHub Actionsの成功ビルドから公開されます。https://github.com/robo1129-ops/sosho-firmware/releases/download/latest/sosho-firmware-latest.zip
スリープ時間ごとのフォルダはありません。書き込み後、SO-SHO Webアプリでオフまたは1〜1440分を保存してください。初期値は30分です。
このフォルダの `sosho-firmware-latest-local.zip` は、6構成をローカルでビルドしてまとめた検証用ZIPです。
言語別の説明は `README-ja.txt` / `README-en.txt` / `README-zh-CN.txt` / `README-ko.txt` /
`README-th.txt` / `README-es.txt` / `README-de.txt` に分けています。通常は最新リリースZIPを使用し、
実機の保存はWebアプリで再読み取り一致を確認してください。

## 書き込むファイル

通常の構成では、右側のCentralへ `sosho-right-trackball.uf2`、左側のPeripheralへ `sosho-left-trackpad.uf2` を書き込みます。

- `sosho-right-trackball.uf2`: 右・Central・トラックボール
- `sosho-left-trackpad.uf2`: 左・Peripheral・トラックパッド
- `sosho-right-trackpad.uf2`: 右・Central・トラックパッド構成を使う場合
- `sosho-right-none.uf2`: 右側をブランクのダミーモジュールにする場合

## トラックパッドの操作

- 中央付近で通常に動かす: ポインター移動
- 短く触れて離す: 左クリック
- 短いタップを2回: 左クリック2回を送り、OSのダブルクリックとして動作
- 右クリック・長押し右ドラッグ: 今回の安定版では非対応
- 外周帯から触れ始めて円周方向になぞる: 今回の安定版では無効（相対モードでは外周判定できないため、右トラックボールのスクロールレイヤーを使用）

今回の安定版では、ポインター無反応の原因になった絶対座標ベースの処理を通常経路から外しています。Pinnacle公式relative modeの移動とprimary tapを使うため、接触位置を変えてもポインターが瞬間移動しません。左側トラックパッドはX軸、右側トラックパッドはX/Y両軸を実装向きに合わせて反転しています。右クリック、長押し右ドラッグ、外周スクロールは今回の安定版では含めません。

## 注意

半角／全角は、Windows US配列・日本語IMEで確実に切り替えられるよう、初期配列とWebアプリの人間向け項目を左Alt+Graveとして扱います。明示的なLANG5は別のキーコードとして保持します。

この版は全6構成をローカルビルド済みです。右トラックボール版は右側へ書き込み後、XIAO-BOOTの切断と通常USB（キーボード・マウス・COM11）への復帰まで確認しています。書き込み前に、現在使っているUF2を保存してください。
