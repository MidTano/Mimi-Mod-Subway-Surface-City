param(
    [Parameter(Mandatory)][string]$SplitApk,
    [Parameter(Mandatory)][string]$OutputDir
)

Add-Type -AssemblyName System.IO.Compression.FileSystem

if (-not (Test-Path $SplitApk)) {
    Write-Host "[extract_split_libs] file not found: $SplitApk"
    exit 1
}

$zip = [System.IO.Compression.ZipFile]::OpenRead($SplitApk)
$count = 0
try {
    foreach ($entry in $zip.Entries) {
        if ($entry.FullName -match '^lib/.+\.so$' -and $entry.Length -gt 0) {
            $dest = Join-Path $OutputDir $entry.FullName
            $destDir = Split-Path $dest -Parent
            if (-not (Test-Path $destDir)) {
                New-Item -ItemType Directory -Path $destDir -Force | Out-Null
            }
            $stream = $entry.Open()
            $file = [System.IO.File]::Create($dest)
            try {
                $stream.CopyTo($file)
                $count++
            } finally {
                $file.Close()
                $stream.Close()
            }
        }
    }
} finally {
    $zip.Dispose()
}

Write-Host "[extract_split_libs] extracted $count .so files from $(Split-Path $SplitApk -Leaf)"
