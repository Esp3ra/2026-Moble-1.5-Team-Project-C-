param(
    [ValidateSet('Debug', 'Release')][string]$Configuration = 'Release',
    [switch]$Rebuild,
    [switch]$SkipTests
)
$ErrorActionPreference = 'Stop'
$projectRoot = $PSScriptRoot
if (-not (Test-Path -LiteralPath (Join-Path $projectRoot 'external\SFML-3.1.0'))) {
    & (Join-Path $projectRoot 'Prepare-Dependencies.ps1')
}
& (Join-Path $projectRoot 'Validate-Dependencies.ps1')
$vswherePath = Join-Path ${env:ProgramFiles(x86)} 'Microsoft Visual Studio\Installer\vswhere.exe'
$msbuildPath = $null
if (Test-Path -LiteralPath $vswherePath) {
    $msbuildPath = & $vswherePath -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -find 'MSBuild\**\Bin\MSBuild.exe' | Select-Object -First 1
}
if (-not $msbuildPath) {
    $msbuildCommand = Get-Command MSBuild.exe -ErrorAction SilentlyContinue
    if ($msbuildCommand) { $msbuildPath = $msbuildCommand.Source }
}
if (-not $msbuildPath) { throw 'Visual Studio 2026 C++ desktop workload (v145) was not found.' }
$buildTarget = if ($Rebuild) { 'Rebuild' } else { 'Build' }
& $msbuildPath (Join-Path $projectRoot 'LevelDevil.slnx') '/m' '/nologo' '/v:minimal' "/t:$buildTarget" "/p:Configuration=$Configuration" '/p:Platform=x64'
if ($LASTEXITCODE -ne 0) { throw "Build failed with exit code $LASTEXITCODE." }
if (-not $SkipTests) {
    & (Join-Path $projectRoot "bin\$Configuration\GameModelTests.exe")
    if ($LASTEXITCODE -ne 0) { throw "Game model tests failed with exit code $LASTEXITCODE." }
}
Write-Host "Ready: $(Join-Path $projectRoot "bin\$Configuration\LevelDevil.exe")" -ForegroundColor Green
