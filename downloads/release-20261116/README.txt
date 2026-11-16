SO-SHO firmware / 2026-11-16 / source commit 4c8a947

Right: trackball / Central
Left: trackpad / Peripheral
Sleep: off

right-trackball/sosho-right-trackball.uf2: flash to RIGHT only.
left-trackpad/sosho-left-trackpad.uf2: flash to LEFT only.

右トラックボール・左トラックパッド、スリープオフ用です。
現在のキーマップ設定はこのパッケージには含まれず、変更しません。
各側をブートローダーモードにして、対応するUF2だけを書き込んでください。
エンコーダー回転時の不正メモリーアクセスと、保存済みレイヤー順序の移行処理を含むローカルビルドです。
左右とも同じソースコミットからローカルビルドしています。

SHA-256
Right: 8C67AFB52945E4304F4E99DB5DEBA1570DB460D93DAF765A8866B060E166F1AB
Left: E3A71F57B7748C825CCDD9C0410FDA27ED5145A555C91940780B85F9647DEADD
