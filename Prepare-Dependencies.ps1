param()
$ErrorActionPreference = 'Stop'
$ProgressPreference = 'SilentlyContinue'
$externalDirectory = Join-Path $PSScriptRoot 'external'
$sfmlDirectory = Join-Path $externalDirectory 'SFML-3.1.0'
$archivePath = Join-Path $externalDirectory 'SFML-3.1.0-windows-vc17-64-bit.zip'
$officialUrl = 'https://www.sfml-dev.org/files/SFML-3.1.0-windows-vc17-64-bit.zip'
$expectedHash = '2D32D05591AE218EEAE67DA32C84435877DB52D6E276D60CC2CC9C8ADB5F4152'
if (Test-Path -LiteralPath $sfmlDirectory) {
    & (Join-Path $PSScriptRoot 'Validate-Dependencies.ps1')
    Write-Host 'SFML 3.1.0 is already available.'
    return
}
New-Item -ItemType Directory -Path $externalDirectory -Force | Out-Null
if (-not (Test-Path -LiteralPath $archivePath)) {
    Write-Host 'Downloading official SFML 3.1.0 (Visual C++ 2022, x64)...'
    Invoke-WebRequest -Uri $officialUrl -OutFile $archivePath
}
$sha256 = [System.Security.Cryptography.SHA256]::Create()
$archiveStream = [System.IO.File]::OpenRead($archivePath)
try {
    $actualHash = [System.BitConverter]::ToString($sha256.ComputeHash($archiveStream)).Replace('-', '')
}
finally {
    $archiveStream.Dispose()
    $sha256.Dispose()
}
if ($actualHash -ne $expectedHash) {
    throw 'SFML archive checksum differs from the verified package. The archive was preserved; check it before replacing it.'
}
Add-Type -AssemblyName System.IO.Compression.FileSystem
[System.IO.Compression.ZipFile]::ExtractToDirectory($archivePath, $externalDirectory)
if (-not (Test-Path -LiteralPath (Join-Path $sfmlDirectory 'include\SFML\Graphics.hpp'))) {
    throw 'SFML extraction did not produce the required headers.'
}
& (Join-Path $PSScriptRoot 'Validate-Dependencies.ps1')
Write-Host 'SFML 3.1.0 is ready.' -ForegroundColor Green
