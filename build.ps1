# Resolve script directory and switch to it
$scriptDir = Split-Path -Path $MyInvocation.MyCommand.Definition -Parent
Set-Location $scriptDir

# Ensure script stops on error
$ErrorActionPreference = "Stop"

# Check for --nodebug flag
$skipDebug = $false
if ($args -contains "--nodebug") {
    $skipDebug = $true
    Write-Host "`n--nodebug flag detected. Skipping Debug builds..." -ForegroundColor Yellow
}

Write-Host "Installing VSSetup module if not already present..." -ForegroundColor Cyan
if (-not (Get-Module -ListAvailable -Name VSSetup)) {
    Install-Module VSSetup -Scope CurrentUser -Force -AllowClobber
}

Import-Module VSSetup

Write-Host "Retrieving installed Visual Studio instances..." -ForegroundColor Cyan
$vsInstance = Get-VSSetupInstance | Sort-Object InstallationVersion -Descending | Select-Object -First 1

if (-not $vsInstance) {
    Write-Error "No Visual Studio instances found."
    exit 1
}

$vsPath = $vsInstance.InstallationPath
$msbuildPath = Join-Path $vsPath "MSBuild\Current\Bin\MSBuild.exe"

if (-not (Test-Path $msbuildPath)) {
    Write-Error "MSBuild not found at expected path: $msbuildPath"
    exit 1
}

Write-Host "Using MSBuild at: $msbuildPath" -ForegroundColor Green

# Define solution and build configurations
$solution = "Winhider.sln"
$configurations = if ($skipDebug) { @("Release") } else { @("Debug", "Release") }
$platforms = @("x86", "x64")

# Status dictionary
$buildStatus = @{}

foreach ($config in $configurations) {
    foreach ($platform in $platforms) {
        $key = "$platform-$config"
        Write-Host "`nBuilding $solution - Configuration: $config, Platform: $platform" -ForegroundColor Cyan
        & "$msbuildPath" $solution /p:Configuration=$config /p:Platform=$platform -m
        if ($LASTEXITCODE -eq 0) {
            $buildStatus[$key] = "Success"
        } else {
            $buildStatus[$key] = "Failed (Exit Code: $LASTEXITCODE)"
        }
    }
}

Write-Host "`n=== Build Summary ===" -ForegroundColor Yellow
foreach ($entry in $buildStatus.GetEnumerator()) {
    $color = if ($entry.Value -like "Success*") { "Green" } else { "Red" }
    Write-Host ("{0,-15} : {1}" -f $entry.Key, $entry.Value) -ForegroundColor $color
}

pause
exit 0
