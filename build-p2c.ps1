param([switch]$UseStaged)

$ErrorActionPreference = 'Stop'
$root = $PSScriptRoot
$manifestPath = Join-Path $root 'output/p2c-runtime.json'
$binaries = @(
    @{ Target = 'x86_64-pc-windows-msvc'; Path = 'output/weasel_p2c.dll'; Machine = 0x8664 },
    @{ Target = 'i686-pc-windows-msvc'; Path = 'output/Win32/weasel_p2c.dll'; Machine = 0x014c }
)
$commit = (& git -C $root rev-parse HEAD).Trim()
if ($LASTEXITCODE -ne 0) { throw 'Cannot determine source commit' }

function Assert-PeMachine($Path, $Expected) {
    $bytes = [IO.File]::ReadAllBytes($Path)
    if ($bytes.Length -lt 64 -or $bytes[0] -ne 0x4d -or $bytes[1] -ne 0x5a) {
        throw "Invalid DLL: $Path"
    }
    $offset = [BitConverter]::ToInt32($bytes, 0x3c)
    if ($offset -lt 0 -or $offset + 6 -gt $bytes.Length) { throw "Invalid PE header: $Path" }
    if ([BitConverter]::ToUInt32($bytes, $offset) -ne 0x4550 -or
        [BitConverter]::ToUInt16($bytes, $offset + 4) -ne $Expected) {
        throw "Incorrect DLL architecture: $Path"
    }
}

if ($UseStaged) {
    $manifest = Get-Content -Raw $manifestPath | ConvertFrom-Json
    if ($manifest.commit -ne $commit) { throw 'Staged DLLs belong to a different source commit' }
    foreach ($binary in $binaries) {
        $path = Join-Path $root $binary.Path
        Assert-PeMachine $path $binary.Machine
        $hash = (Get-FileHash $path -Algorithm SHA256).Hash
        $entry = @($manifest.binaries | Where-Object { $_.target -eq $binary.Target })
        if ($entry.Count -ne 1 -or $entry[0].sha256 -ne $hash) {
            throw "Staged DLL checksum mismatch: $path"
        }
    }
    Write-Host "Verified bundled P2C DLLs for $commit"
    exit 0
}

Push-Location (Join-Path $root 'weasel-neural')
try {
    & rustup target add x86_64-pc-windows-msvc i686-pc-windows-msvc
    if ($LASTEXITCODE -ne 0) { throw 'Could not install Rust targets' }
    $entries = @()
    foreach ($binary in $binaries) {
        & cargo build --release --locked --lib --target $binary.Target --target-dir (Join-Path $root 'weasel-neural/target')
        if ($LASTEXITCODE -ne 0) { throw "Rust build failed: $($binary.Target)" }
        $source = Join-Path (Get-Location).Path "target/$($binary.Target)/release/weasel_p2c.dll"
        $destination = Join-Path $root $binary.Path
        New-Item -ItemType Directory -Force -Path (Split-Path $destination) | Out-Null
        Copy-Item $source $destination -Force
        Assert-PeMachine $destination $binary.Machine
        $entries += @{ target = $binary.Target; sha256 = (Get-FileHash $destination -Algorithm SHA256).Hash }
    }
    @{ commit = $commit; binaries = $entries } | ConvertTo-Json -Depth 4 |
        Set-Content -Encoding UTF8 $manifestPath
} finally {
    Pop-Location
}
