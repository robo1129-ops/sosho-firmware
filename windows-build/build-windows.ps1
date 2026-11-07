param(
  [ValidateSet('sosho_R_trackball','sosho_R_trackpad','sosho_R_none')]
  [string]$Shield = 'sosho_R_trackball',
  [switch]$Clean,
  [switch]$NoUpdate
)

$ErrorActionPreference = 'Stop'
$root = (Resolve-Path (Join-Path $PSScriptRoot '..')).Path
$pythonCommand = Get-Command python -ErrorAction Stop
$pythonHome = (& $pythonCommand.Source -c "import sys; print(sys.prefix)").Trim()
$pythonUserBase = (& $pythonCommand.Source -c "import site; print(site.getuserbase())").Trim()
$pythonVersion = (& $pythonCommand.Source -c "import sys; print(f'Python{sys.version_info.major}{sys.version_info.minor}')").Trim()
$pythonScriptPaths = @(
  (Join-Path $pythonHome 'Scripts'),
  (Join-Path $pythonUserBase 'Scripts'),
  (Join-Path (Join-Path $pythonUserBase $pythonVersion) 'Scripts')
) | Where-Object { Test-Path $_ }
$pythonScriptPaths += Get-ChildItem (Join-Path $pythonUserBase '*\Scripts') -Directory -ErrorAction SilentlyContinue | ForEach-Object { $_.FullName }
$pythonScriptPaths = $pythonScriptPaths | Select-Object -Unique
$toolchainCandidates = @(
  'C:\Program Files (x86)\Arm GNU Toolchain arm-none-eabi\12.2 mpacbti-rel1\bin'
)
$toolchainCandidates += Get-ChildItem 'C:\Program Files*\Arm GNU Toolchain arm-none-eabi\*\bin' -Directory -ErrorAction SilentlyContinue | ForEach-Object { $_.FullName }
$armToolchain = $toolchainCandidates | Where-Object { Test-Path (Join-Path $_ 'arm-none-eabi-gcc.exe') } | Select-Object -First 1
if (-not $armToolchain) { throw 'Arm GNU Toolchain arm-none-eabi-gcc.exe was not found. Install the official toolchain.' }
$env:Path = (($pythonScriptPaths + $armToolchain + $env:Path) -join ';')
$env:ZEPHYR_TOOLCHAIN_VARIANT = 'gnuarmemb'
$env:GNUARMEMB_TOOLCHAIN_PATH = Split-Path $armToolchain

function Invoke-West {
  & python -m west @args
  if ($LASTEXITCODE -ne 0) { throw "west failed with exit code $LASTEXITCODE" }
}

function Apply-PatchIfNeeded([string]$repo, [string]$patch) {
  $previousErrorAction = $ErrorActionPreference
  $ErrorActionPreference = 'Continue'
  $null = & git -C $repo apply --check $patch 2>&1
  $checkCode = $LASTEXITCODE
  if ($checkCode -eq 0) {
    $null = & git -C $repo apply $patch 2>&1
    $applyCode = $LASTEXITCODE
    $ErrorActionPreference = $previousErrorAction
    if ($applyCode -ne 0) { throw "patch failed: $patch" }
    return
  }
  $null = & git -C $repo apply --reverse --check $patch 2>&1
  $reverseCode = $LASTEXITCODE
  $ErrorActionPreference = $previousErrorAction
  if ($reverseCode -eq 0) { return }
  throw "patch is neither applicable nor already applied: $patch"
}

Push-Location $root
try {
  if (-not (Test-Path (Join-Path $root '.west'))) {
    Invoke-West init -l (Join-Path $root 'config')
  }
  if (-not $NoUpdate) {
    Invoke-West update --fetch-opt=--filter=tree:0
  }

  $zmk = Join-Path $root 'zmk'
  if (Test-Path $zmk) {
    & git -C $zmk reset --hard HEAD
    & git -C $zmk clean -fdx
    Apply-PatchIfNeeded $zmk (Join-Path $root 'patches\zmk-core-runtime.patch')
    Apply-PatchIfNeeded $zmk (Join-Path $root 'patches\zmk-hold-tap-same-key-hold.patch')
    Apply-PatchIfNeeded $zmk (Join-Path $root 'patches\zmk-studio-usb-uart.patch')
  }
  $pmw = Join-Path $root 'modules\zmk-pmw3610-driver'
  if (Test-Path $pmw) {
    Apply-PatchIfNeeded $pmw (Join-Path $root 'patches\pmw3610-battery-report-rate.patch')
  }
  $messages = Join-Path $root 'modules\msgs\zmk-studio-messages'
  if (Test-Path $messages) {
    & git -C $messages reset --hard HEAD
    & git -C $messages clean -fdx
    Apply-PatchIfNeeded $messages (Join-Path $root 'patches\zmk-studio-messages.patch')
  }

  Invoke-West zephyr-export
  $buildDir = Join-Path $root ("build-windows\{0}" -f $Shield)
  if ($Clean -and (Test-Path $buildDir)) { Remove-Item -LiteralPath $buildDir -Recurse -Force }
  $extraModules = @(
    (Join-Path $root 'modules\zmk-pmw3610-driver'),
    (Join-Path $root 'modules\zmk-rgbled-widget'),
    (Join-Path $root 'modules\zmk_encoder')
  ) -join ';'
  $cmakeArgs = @(
    "-DZephyr_DIR=$(Join-Path $root 'zephyr\share\zephyr-package\cmake')",
    "-DBOARD_ROOT=$root",
    "-DDTS_ROOT=$root",
    "-DZMK_CONFIG=$(Join-Path $root 'config')",
    "-DZMK_EXTRA_MODULES=$extraModules",
    "-DSHIELD=$Shield",
    '-DCONFIG_ZMK_STUDIO=y',
    '-DCONFIG_ZMK_STUDIO_TRANSPORT_BLE=y',
    '-DCONFIG_ZMK_SLEEP=y',
    '-DCONFIG_ZMK_IDLE_SLEEP_TIMEOUT=1800000'
  )
  $westBuildArgs = @(
    'build', '-s', (Join-Path $root 'zmk\app'), '-d', $buildDir,
    '-b', 'xiao_ble/nrf52840/zmk', '-S', 'studio-rpc-usb-uart', '--'
  ) + $cmakeArgs
  Invoke-West @westBuildArgs
  $artifact = Join-Path $buildDir 'zephyr\zmk.uf2'
  if (-not (Test-Path $artifact)) { throw "UF2 was not generated: $artifact" }
  Write-Host "Windows native ZMK build succeeded: $artifact" -ForegroundColor Green
} finally {
  Pop-Location
}
