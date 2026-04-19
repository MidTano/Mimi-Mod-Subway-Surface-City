param([Parameter(Mandatory)][string]$ManifestPath)

if (-not (Test-Path $ManifestPath)) {
    Write-Host "[patch_manifest] file not found: $ManifestPath"
    exit 1
}

$m = Get-Content $ManifestPath -Raw -Encoding UTF8

$m = $m -replace 'android:isSplitRequired="true"', 'android:isSplitRequired="false"'
$m = $m -replace 'android:requiredSplitTypes="[^"]*"', ''
$m = $m -replace '<meta-data android:name="com.android.vending.splits.required" android:value="true"/>', '<meta-data android:name="com.android.vending.splits.required" android:value="false"/>'
$m = $m -replace '<meta-data android:name="com.android.vending.splits" android:resource="@xml/splits0"/>', ''

Set-Content $ManifestPath $m -NoNewline -Encoding UTF8
Write-Host "[patch_manifest] split requirements removed"
