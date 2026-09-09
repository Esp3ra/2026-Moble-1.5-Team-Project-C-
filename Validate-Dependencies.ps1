param()
$ErrorActionPreference = 'Stop'
$headerDirectory = Join-Path $PSScriptRoot 'external\SFML-3.1.0\include\SFML'
# SHA-256 values from the verified, unmodified official SFML 3.1.0 package.
$expectedHeaders = [ordered]@{
    'Config.hpp'        = 'F271118976C0F1F2FD10449DFB49602831668B04726AD882726A071035AE8254'
    'GpuPreference.hpp' = '0ED09F0A7FDF812038BB8C94CC545C1FAA968640219DD9AE7FA89678F5A32F3B'
    'Graphics.hpp'      = '068F41F6B38A1D27E73D6C6CC0CD2BE1E86F25D3FB695C1A9ACE791F983D3F92'
    'Main.hpp'          = '925ED17580F6D43310F739BC0E59EFEA4160E545339B9526C5BE2455D7B4558A'
    'Network.hpp'       = 'B5D6A50EDFCEA27991A7F5EC2BCCC40ADFCE0402E90C81A7ECA1FD8F565F8BA3'
    'OpenGL.hpp'        = '4D54874DDD2400022015AEA4857074D8CCFE1AE0D35671FBB24B66D0FB21B876'
    'System.hpp'        = '85E05E8A87C3C1E888650EBAD1353BE8473E2889BF317836BB9FA848E7D0A9C5'
    'Window.hpp'        = '7C9C9048AE75B3784DF674779EFE6C077EE02A9301AE333690879434B72A91C1'
}
foreach ($header in $expectedHeaders.GetEnumerator()) {
    $headerPath = Join-Path $headerDirectory $header.Key
    if (-not (Test-Path -LiteralPath $headerPath -PathType Leaf)) {
        throw "SFML header is missing: $($header.Key). Back up local dependency changes, then restore the original header from the verified official ZIP. No files were overwritten."
    }
    $sha256 = [System.Security.Cryptography.SHA256]::Create()
    $headerStream = [System.IO.File]::OpenRead($headerPath)
    try {
        $actualHash = [System.BitConverter]::ToString($sha256.ComputeHash($headerStream)).Replace('-', '')
    }
    finally {
        $headerStream.Dispose()
        $sha256.Dispose()
    }
    if ($actualHash -ne $header.Value) {
        throw "SFML header differs from the official package: $($header.Key). Back up this file, then restore the original header from the verified official ZIP. No files were overwritten."
    }
}
