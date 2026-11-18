SO-SHO / 2026-11-19 / source 74b13d8
Right trackball Central / Left trackpad Peripheral

無操作30分後に省電力へ移行。スリープは無効です。
一時的な送信待ちのキー通知は押下・解放の順序で保持・再送します。
保持容量には上限があり、切断中の入力を無制限に記憶する機能ではありません。
左右ともローカルビルド・実機書き込み済み。30分待機後の復帰テストは未確認です。
現在の保存済みキー設定を初期化する必要はありません。

右用: right-trackball/sosho-right-trackball.uf2
左用: left-trackpad/sosho-left-trackpad.uf2
対応する側のUF2のみを書き込んでください。

Idle power saving after 30 minutes; deep sleep disabled.
Transiently unsent key reports are retained in order within bounded queues.
Both binaries were built locally and flashed. The 30-minute wake test remains pending.
Flash only the file matching each half. No settings reset is needed.

SHA-256
Right: 28E6D8AC5B7E9E388DEB3F6751D828BD5610B51F247A0E8675359DA6204132B9
Left: 85F866C9E847CA46A9FB36284821341ACA56F82D4968DFEB471BA866AA0ECFE6
