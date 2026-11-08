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

半角／全角は、初期配列とWebアプリの選択・ドラッグ＆ドロップ・RPC書き込みをすべてHID `LANG5`（usage `0x94`）へ統一しています。ホストOSや日本語IMEの設定によって実際のIME切り替え動作は異なる場合があります。

この版は全6構成をローカルビルド済みです。右トラックボール版は右側へ書き込み後、XIAO-BOOTの切断と通常USB（キーボード・マウス・COM11）への復帰まで確認しています。書き込み前に、現在使っているUF2を保存してください。

## Bluetooth latency correction

The left Peripheral firmware keeps the 6–12 connection interval and gives
split input notifications priority. The right Central firmware explicitly uses
slave latency 0. The older default latency 30 could make left-half pointing
input arrive in bursts, so update both halves from the same release before
judging latency.

The published sleep-off pair is built from the same source and keeps the
existing keymap, macros, encoders, pointing controls, and Studio transports.

## 2026-11-05 ホールドレイヤー修正

ホールドによるレイヤー移動が一時的に途切れたように見える問題を修正しました。左右分割の送信元を含めて同一キーを判定し、直前にタップしたレイヤー移動キーを次の長押しで誤ってタップ確定しないようにしています。レイヤータップは押下直後からホールド側を有効化し、タップだった場合だけ解除してからタップを送るため、レイヤー保持時の判定待ちが入力停止のように見える状態を避けます。右Central用の修正版は `sosho-right-trackball.uf2` です。

## 2026-11-07 統合修正

センサー共通の固定起動待ちとPMW3610の復帰時初期待ちをともに40msへ短縮し、無線復帰後に約1秒入力が止まる症状を緩和しました。タップ後の同じキー再ホールド、ホールド側がレイヤー移動のキー、LANG5半角／全角、BT接続先操作を含むマクロを同じ形式で扱います。マクロは最大64ステップ、待ち時間の初期値は1msです。実機書き込み後は必ずWebアプリで読み取り→変更→保存→再読み取りを行い、一致表示を確認してください。
