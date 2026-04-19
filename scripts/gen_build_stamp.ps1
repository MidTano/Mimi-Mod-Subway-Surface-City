param(
    [string]$OutputPath = "src/core/runtime/build_stamp_gen.hpp"
)

$scriptDir = Split-Path -Parent $MyInvocation.MyCommand.Path
$repoRoot  = Resolve-Path (Join-Path $scriptDir "..")
$target    = Join-Path $repoRoot $OutputPath

$targetDir = Split-Path -Parent $target
if (-not (Test-Path $targetDir)) {
    New-Item -ItemType Directory -Force -Path $targetDir | Out-Null
}

$date  = Get-Date -Format "yyyy-MM-dd"
$time  = Get-Date -Format "HH:mm:ss"
$tz    = (Get-Date).ToString("zzz")
$epoch = [int64](Get-Date -UFormat %s)

$guid     = [guid]::NewGuid().ToString("N")
$id       = $guid.Substring(0, 8).ToUpper()
$stampStr = ("{0} {1} {2}" -f $date, $time, $tz)

$lines = @()
$lines += "#pragma once"
$lines += ""
$lines += "namespace build_stamp {"
$lines += ""
$lines += ("constexpr const char* kDate      = ""{0}"";" -f $date)
$lines += ("constexpr const char* kTime      = ""{0}"";" -f $time)
$lines += ("constexpr const char* kTz        = ""{0}"";" -f $tz)
$lines += ("constexpr const char* kStamp     = ""{0}"";" -f $stampStr)
$lines += ("constexpr const char* kId        = ""{0}"";" -f $id)
$lines += ("constexpr long long   kEpochSec  = {0}LL;"  -f $epoch)
$lines += ""
$lines += "}"
$lines += ""

$content = ($lines -join "`r`n")

$existing = $null
if (Test-Path $target) {
    $existing = [System.IO.File]::ReadAllText($target)
}

if ($existing -ne $content) {
    [System.IO.File]::WriteAllText($target, $content)
    Write-Host "[gen_build_stamp] wrote $target (id=$id ts=$stampStr)"
} else {
    Write-Host "[gen_build_stamp] unchanged $target"
}
