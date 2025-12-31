param(
    [string]$BuildDir = "",
    [string]$BuildType = "Release",
    [string]$VisualStudioVersion = "",
    [string]$VcpkgDir = "",
    [string]$VcpkgBuildTrees = ""
)

$ErrorActionPreference = "Stop"

function Write-Error-Exit {
    param([string]$Message)
    Write-Host "ERROR: $Message" -ForegroundColor Red
    exit 1
}

function Test-Command {
    param([string]$Command)
    return [bool](Get-Command $Command -ErrorAction SilentlyContinue)
}

function Get-VisualStudio {
    $vsWhere = "${env:ProgramFiles(x86)}\Microsoft Visual Studio\Installer\vswhere.exe"
    $instances = @()

    if (Test-Path $vsWhere) {
        try {
            $oldEncoding = [Console]::OutputEncoding
            [Console]::OutputEncoding = [System.Text.Encoding]::UTF8

            $json = & $vsWhere -all -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -format json -utf8

            [Console]::OutputEncoding = $oldEncoding

            if ($json) {
                $rawInstances = $json | ConvertFrom-Json
                if ($rawInstances -is [System.Management.Automation.PSCustomObject]) {
                    $rawInstances = @($rawInstances)
                }

                foreach ($item in $rawInstances) {
                    $majorVersion = $item.installationVersion.Split('.')[0]
                    $generator = ""
                    switch ($majorVersion) {
                        "18" { $generator = "Visual Studio 18 2026" }
                        "17" { $generator = "Visual Studio 17 2022" }
                        "16" { $generator = "Visual Studio 16 2019" }
                        "15" { $generator = "Visual Studio 15 2017" }
                        Default { $generator = "Visual Studio $majorVersion" }
                    }

                    $instances += @{
                        Path = $item.installationPath
                        Version = $item.installationVersion
                        Generator = $generator
                    }
                }
            }
        } catch {
            [Console]::OutputEncoding = $oldEncoding
        }
    }

    if ($instances.Count -eq 0) {
        $fallback = @(
            @{Path = "${env:ProgramFiles}\Microsoft Visual Studio\2022\Community"; Version = "17.0"; Generator = "Visual Studio 17 2022"},
            @{Path = "${env:ProgramFiles}\Microsoft Visual Studio\2019\Community"; Version = "16.0"; Generator = "Visual Studio 16 2019"}
        )

        foreach ($item in $fallback) {
            if (Test-Path $item.Path) {
                $instances += $item
            }
        }
    }

    if ($instances.Count -eq 0) { return $null }

    if ($VisualStudioVersion) {
        $selected = $instances | Where-Object { $_.Version.StartsWith($VisualStudioVersion) } | Sort-Object Version -Descending | Select-Object -First 1
        if ($selected) { return $selected }
        Write-Host "Warning: VS version '$VisualStudioVersion' not found, using latest" -ForegroundColor Yellow
    }

    return $instances | Sort-Object Version -Descending | Select-Object -First 1
}

# Initialize paths
$SrcDir = Split-Path -Parent $PSScriptRoot
if (-not $BuildDir) { $BuildDir = Join-Path $env:USERPROFILE "build\apvlv" }
$BuildDir = $ExecutionContext.SessionState.Path.GetUnresolvedProviderPathFromPSPath($BuildDir)

if ($VcpkgDir) {
    $VcpkgRoot = $ExecutionContext.SessionState.Path.GetUnresolvedProviderPathFromPSPath($VcpkgDir)
} else {
    $VcpkgRoot = Join-Path $BuildDir "vcpkg"
}
$VcpkgInstalledDir = Join-Path $BuildDir "vcpkg_installed"
$BuildDirPath = Join-Path $BuildDir "build"
if ($VcpkgBuildTrees) {
    $VcpkgBuildTreesPath = $ExecutionContext.SessionState.Path.GetUnresolvedProviderPathFromPSPath($VcpkgBuildTrees)
} else {
    $VcpkgBuildTreesPath = ""
}

Write-Host "Build Configuration:" -ForegroundColor Cyan
Write-Host "  Build Directory: $BuildDir"
Write-Host "  Build Type: $BuildType"
Write-Host "  Source Directory: $SrcDir"

# Check dependencies
Write-Host "Checking dependencies..." -ForegroundColor Green
if (-not (Test-Command "git")) { Write-Error-Exit "Git not found" }
if (-not (Test-Command "cmake")) { Write-Error-Exit "CMake not found" }

$vs = Get-VisualStudio
if (-not $vs) { Write-Error-Exit "Visual Studio with C++ tools not found" }
Write-Host "Using: $($vs.Generator)" -ForegroundColor Green

# Create build directory
if (!(Test-Path $BuildDir)) {
    Write-Host "Creating build directory..." -ForegroundColor Green
    New-Item -ItemType Directory -Path $BuildDir | Out-Null
}

# Setup vcpkg
if (!(Test-Path $VcpkgRoot) -or !(Test-Path "$VcpkgRoot\.git")) {
    Write-Host "Cloning vcpkg..." -ForegroundColor Green
    if (Test-Path $VcpkgRoot) { Remove-Item -Recurse -Force $VcpkgRoot }
    Push-Location $BuildDir
    try {
        git clone https://github.com/microsoft/vcpkg vcpkg
        if ($LASTEXITCODE -ne 0) { Write-Error-Exit "Failed to clone vcpkg" }
    } finally {
        Pop-Location
    }
} else {
    Write-Host "Updating vcpkg..." -ForegroundColor Green
    Push-Location "$VcpkgRoot"
    try {
        $isShallow = (& git rev-parse --is-shallow-repository).Trim()
        if ($isShallow -eq "true") {
            git fetch --unshallow
            if ($LASTEXITCODE -ne 0) { Write-Error-Exit "Failed to unshallow vcpkg repository" }
        }
        git pull --ff-only
        if ($LASTEXITCODE -ne 0) { Write-Error-Exit "Failed to update vcpkg" }
    } finally {
        Pop-Location
    }
}

if (!(Test-Path "$VcpkgRoot\vcpkg.exe")) {
    Write-Host "Bootstrapping vcpkg..." -ForegroundColor Green
    Push-Location "$VcpkgRoot"
    try {
        .\bootstrap-vcpkg.bat
        if ($LASTEXITCODE -ne 0) { Write-Error-Exit "Failed to bootstrap vcpkg" }
    } finally {
        Pop-Location
    }
}

Write-Host "Installing vcpkg dependencies..." -ForegroundColor Green
$env:VCPKG_INSTALLED_DIR = $VcpkgInstalledDir
Push-Location "$SrcDir"
try {
    $installArgs = @("install", "--triplet=x64-windows", "--clean-after-build", "--x-install-root=$VcpkgInstalledDir")
    if ($VcpkgBuildTreesPath) { $installArgs += "--x-buildtrees-root=$VcpkgBuildTreesPath" }
    & "$VcpkgRoot\vcpkg.exe" $installArgs
    if ($LASTEXITCODE -ne 0) { Write-Error-Exit "Failed to install dependencies" }
} finally {
    Pop-Location
}
Remove-Item Env:VCPKG_INSTALLED_DIR -ErrorAction SilentlyContinue

# Configure and build
if (Test-Path $BuildDirPath) {
    Write-Host "Cleaning build directory..." -ForegroundColor Green
    Remove-Item -Recurse -Force $BuildDirPath
}

New-Item -ItemType Directory -Path $BuildDirPath | Out-Null

Write-Host "Configuring with CMake..." -ForegroundColor Green
Push-Location "$BuildDirPath"
try {
    cmake "$SrcDir" `
      -DCMAKE_TOOLCHAIN_FILE="$VcpkgRoot\scripts\buildsystems\vcpkg.cmake" `
      -DVCPKG_TARGET_TRIPLET=x64-windows `
      -DCMAKE_BUILD_TYPE=$BuildType `
      -G "$($vs.Generator)" `
      -A x64
    if ($LASTEXITCODE -ne 0) { Write-Error-Exit "CMake configuration failed" }
    Write-Host "Building project..." -ForegroundColor Green
    $ParallelJobs = [Math]::Min([Environment]::ProcessorCount, 8)
    cmake --build . --config $BuildType --parallel $ParallelJobs
    if ($LASTEXITCODE -ne 0) { Write-Error-Exit "Build failed" }
} finally {
    Pop-Location
}

Write-Host "Build completed successfully!" -ForegroundColor Green
Write-Host "Output: $BuildDirPath\$BuildType" -ForegroundColor Cyan
