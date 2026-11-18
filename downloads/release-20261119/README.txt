SO-SHO firmware / 2026-11-19
Source: 8e9df23

Right: trackball / Central
Left: trackpad / Peripheral
Sleep and idle power transitions: off

右側には right-trackball/sosho-right-trackball.uf2 のみを書き込んでください。
左側には left-trackpad/sosho-left-trackpad.uf2 のみを書き込んでください。
現在の保存済みキー割り当てを初期化する必要はありません。

通知再送、キュー容量、ポインター通信とキー通信の処理配分を修正。
タップ判定280ms、判定前の仮ホールドなし、同一キーのタップ後長押しに対応。
左右ともWindowsでローカルビルド・書き込み済み。
ユーザーより現時点の動作は順調との確認を受けています。

Flash only the UF2 matching each half. No settings reset is required.
Both binaries were built locally and flashed; the user reports normal operation so far.

SHA-256
Right: BB7129A9DB1BAA18A59BF22146ADC823502C33ACA9B8031A955A696264BE3F53
Left: 9A14F819B6E797218051EF1EE0E104FA602DCC727BA5A394CA0A04538602F4A3
