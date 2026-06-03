param(
    [string]$BuildType = "Release",
    [string]$BuildDir = "build",
    [string]$VcpkgRoot = "C:\vcpkg"
)

$ErrorActionPreference = "Stop"
$SrcDir = Split-Path -Parent $PSScriptRoot

if (-not $env:QT_ROOT_DIR) {
    throw "Set QT_ROOT_DIR to your Qt 6 installation (e.g. C:\Qt\6.8.0\msvc2022_64)"
}

if (-not (Test-Path "$VcpkgRoot\vcpkg.exe")) {
    git clone https://github.com/microsoft/vcpkg $VcpkgRoot
    & "$VcpkgRoot\bootstrap-vcpkg.bat" -disableMetrics
}

$overlayTriplets = Join-Path $SrcDir "vcpkg-overlay-triplets"
& "$VcpkgRoot\vcpkg.exe" install --triplet x64-windows --overlay-triplets="$overlayTriplets" libmupdf quazip cmark tesseract
if ($LASTEXITCODE -ne 0) { throw "vcpkg install failed" }

cmake -B $BuildDir -S $SrcDir -G Ninja `
  -DCMAKE_BUILD_TYPE=$BuildType `
  -DCMAKE_TOOLCHAIN_FILE="$VcpkgRoot\scripts\buildsystems\vcpkg.cmake" `
  -DVCPKG_TARGET_TRIPLET=x64-windows `
  -DVCPKG_OVERLAY_TRIPLETS="$overlayTriplets" `
  -DVCPKG_MANIFEST_MODE=OFF `
  -DCMAKE_PREFIX_PATH="$env:QT_ROOT_DIR" `
  -DAPVLV_WITH_DJVU=OFF `
  -DAPVLV_WITH_POPPLER=OFF
if ($LASTEXITCODE -ne 0) { throw "cmake configure failed" }

cmake --build $BuildDir --parallel
