param(
    [string]$BuildType = "Release",
    [string]$BuildDir = "build",
    [string]$VcpkgRoot = "C:\vcpkg",
    [string]$VcpkgCommit = "d015e31e90838a4c9dfa3eed45979bc70d9357fc"
)

$ErrorActionPreference = "Stop"
$SrcDir = Split-Path -Parent $PSScriptRoot
if (-not [System.IO.Path]::IsPathRooted($BuildDir)) {
    $BuildDir = Join-Path $SrcDir $BuildDir
}

if (-not $env:QT_ROOT_DIR) {
    throw "Set QT_ROOT_DIR to your Qt 6 installation (e.g. C:\Qt\6.8.0\msvc2022_64)"
}

if (-not (Test-Path "$VcpkgRoot\vcpkg.exe")) {
    git clone https://github.com/microsoft/vcpkg $VcpkgRoot
    git -C $VcpkgRoot checkout $VcpkgCommit
    & "$VcpkgRoot\bootstrap-vcpkg.bat" -disableMetrics
}

$overlayPorts = Join-Path $BuildDir "vcpkg-overlay-ports"
$overlayTriplets = Join-Path $BuildDir "vcpkg-overlay-triplets"
New-Item -ItemType Directory -Force -Path "$overlayPorts\quazip" | Out-Null
New-Item -ItemType Directory -Force -Path $overlayTriplets | Out-Null
Copy-Item -Recurse -Force "$VcpkgRoot\ports\quazip\*" "$overlayPorts\quazip\"
$manifest = "$overlayPorts\quazip\vcpkg.json"
jq -f "$SrcDir\ci\strip-quazip-qt-deps.jq" $manifest | Set-Content "$manifest.tmp"
Move-Item -Force "$manifest.tmp" $manifest

$qtPrefix = ($env:QT_ROOT_DIR).Replace('\', '/')
(Get-Content "$SrcDir\vcpkg-overlay-triplets\x64-windows.cmake" -Raw).Replace('@APVLV_QT_PREFIX@', $qtPrefix) |
    Set-Content "$overlayTriplets\x64-windows.cmake"

& "$VcpkgRoot\vcpkg.exe" install --triplet x64-windows `
    --overlay-ports="$overlayPorts" `
    --overlay-triplets="$overlayTriplets" `
    libmupdf quazip cmark tesseract
if ($LASTEXITCODE -ne 0) { throw "vcpkg install failed" }

cmake -B $BuildDir -S $SrcDir -G Ninja `
  "-DCMAKE_BUILD_TYPE=$BuildType" `
  -DCMAKE_TOOLCHAIN_FILE="$VcpkgRoot\scripts\buildsystems\vcpkg.cmake" `
  -DVCPKG_TARGET_TRIPLET=x64-windows `
  -DVCPKG_OVERLAY_TRIPLETS="$overlayTriplets" `
  -DVCPKG_MANIFEST_MODE=OFF `
  -DCMAKE_PREFIX_PATH="$env:QT_ROOT_DIR" `
  -DAPVLV_WITH_DJVU=OFF `
  -DAPVLV_WITH_POPPLER=OFF
if ($LASTEXITCODE -ne 0) { throw "cmake configure failed" }

cmake --build $BuildDir --parallel
