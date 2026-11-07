# Windows ZMK build helper

`build-windows.ps1` は、Windows上でPython/west、CMake、Ninja、Arm GNU Toolchainを使って、SO-SHOの右側Central向けUF2をローカル生成します。Pythonの場所とArm GNU Toolchainのインストール場所は自動検出します。

必要なもの:

- Python 3.10以降
- `python -m pip install --user west cmake ninja`
- Arm GNU Toolchain `arm-none-eabi-gcc`
- Git

初回はリポジトリのルートで次を実行します。

```powershell
python -m pip install --user west cmake ninja
Set-ExecutionPolicy -Scope Process Bypass
.\windows-build\build-windows.ps1 -Shield sosho_R_trackball -Clean
```

生成物は `build-windows\sosho_R_trackball\zephyr\zmk.uf2` です。書き込み前にWindowsのエクスプローラーで右側XIAOのブートローダードライブを確認してください。このスクリプトは自動書き込みを行いません。

Windows固有の差異でビルドできない場合は、同じ `config/west.yml` とパッチを使うGitHub ActionsのLinuxコンテナビルドをフォールバックにします。UF2が生成されるまで実機へ書き込まないでください。
