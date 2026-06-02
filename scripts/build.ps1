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

function Invoke-Checked {
    param([scriptblock]$Action, [string]$ErrorMessage)
    & $Action
    if ($LASTEXITCODE -ne 0) { Write-Error-Exit $ErrorMessage }
}

function Use-Location {
    param([string]$Path, [scriptblock]$Action)
    Push-Location $Path
    try { & $Action } finally { Pop-Location }
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
    Use-Location $BuildDir {
        Invoke-Checked { git clone https://github.com/microsoft/vcpkg vcpkg } "Failed to clone vcpkg"
    }
} else {
    Write-Host "Updating vcpkg..." -ForegroundColor Green
    Use-Location $VcpkgRoot {
        $isShallow = (& git rev-parse --is-shallow-repository).Trim()
        if ($isShallow -eq "true") {
            Invoke-Checked { git fetch --unshallow } "Failed to unshallow vcpkg repository"
        }
        Invoke-Checked { git pull --ff-only } "Failed to update vcpkg"
    }
}

if (!(Test-Path "$VcpkgRoot\vcpkg.exe")) {
    Write-Host "Bootstrapping vcpkg..." -ForegroundColor Green
    Use-Location $VcpkgRoot {
        Invoke-Checked { .\bootstrap-vcpkg.bat } "Failed to bootstrap vcpkg"
    }
}

Write-Host "Installing vcpkg dependencies..." -ForegroundColor Green
$env:VCPKG_INSTALLED_DIR = $VcpkgInstalledDir
$installArgs = @("install", "--triplet=x64-windows", "--clean-after-build", "--x-install-root=$VcpkgInstalledDir")
if ($VcpkgBuildTreesPath) { $installArgs += "--x-buildtrees-root=$VcpkgBuildTreesPath" }
Use-Location $SrcDir {
    Invoke-Checked { & "$VcpkgRoot\vcpkg.exe" $installArgs } "Failed to install dependencies"
}
Remove-Item Env:VCPKG_INSTALLED_DIR -ErrorAction SilentlyContinue

# Configure and build
if (Test-Path $BuildDirPath) {
    Write-Host "Cleaning build directory..." -ForegroundColor Green
    Remove-Item -Recurse -Force $BuildDirPath
}

New-Item -ItemType Directory -Path $BuildDirPath | Out-Null

Write-Host "Configuring with CMake..." -ForegroundColor Green
$ParallelJobs = [Math]::Min([Environment]::ProcessorCount, 8)
Use-Location $BuildDirPath {
    Invoke-Checked {
        cmake "$SrcDir" `
          -DCMAKE_TOOLCHAIN_FILE="$VcpkgRoot\scripts\buildsystems\vcpkg.cmake" `
          -DVCPKG_TARGET_TRIPLET=x64-windows `
          -DCMAKE_BUILD_TYPE=$BuildType `
          -G "$($vs.Generator)" `
          -A x64
    } "CMake configuration failed"
    Write-Host "Building project..." -ForegroundColor Green
    Invoke-Checked { cmake --build . --config $BuildType --parallel $ParallelJobs } "Build failed"
}

Write-Host "Build completed successfully!" -ForegroundColor Green
Write-Host "Output: $BuildDirPath\$BuildType" -ForegroundColor Cyan
