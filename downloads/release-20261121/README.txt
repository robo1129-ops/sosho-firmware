SO-SHO / 2026-11-21
Right trackball Central: source 773a31a
Left trackpad Peripheral: unchanged from source 74b13d8

無操作30分で省電力へ移行し、キーボード全体のスリープは無効です。
右トラックボールのセンサー独自の早期省電力移行を抑制しました。
左側に今回のセンサー修正は不要のため、最新の書き込み済み版を同梱しています。
入力安定化、タップ判定280ms、タップ後の同一キー長押しの修正を含みます。
左右ともローカルビルド・書き込み済み。30分待機後の復帰テストは未確認です。
現在の保存済みキー割り当てを初期化する必要はありません。

Right only: right-trackball/sosho-right-trackball.uf2
Left only: left-trackpad/sosho-left-trackpad.uf2
Idle power saving after 30 minutes; keyboard deep sleep disabled.
The right sensor stays awake until the idle transition. The left binary is unchanged.
Both files were built locally and flashed. The 30-minute wake test remains pending.
Flash only the matching half; no settings reset is required.

SHA-256
Right: ADA95281D1B83631BDBD195CF436641C17E5FA795CA638E5A3125F00D8A46EF4
Left: 85F866C9E847CA46A9FB36284821341ACA56F82D4968DFEB471BA866AA0ECFE6
