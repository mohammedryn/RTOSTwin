$ErrorActionPreference = "Stop"

$repoRoot = Split-Path -Parent (Split-Path -Parent $PSScriptRoot)
$targets = @(
    (Join-Path $repoRoot 'vnv_final\agent\main.c'),
    (Join-Path $repoRoot 'vnv_final\agent\core\snapshot.c'),
    (Join-Path $repoRoot 'vnv_final\agent\core\encoder.c'),
    (Join-Path $repoRoot 'vnv_final\agent\core\framer.c'),
    (Join-Path $repoRoot 'vnv_final\agent\core\transport.c')
)

$allocationCallPattern = '\b(malloc|calloc|realloc|free|pvPortMalloc|pvPortFree)\s*\('
$matchesFound = $false

foreach ($file in $targets) {
    $hits = Select-String -Path $file -Pattern $allocationCallPattern
    if ($hits) {
        $matchesFound = $true
        $hits | ForEach-Object { Write-Host $_ }
    }
}

if ($matchesFound) {
    throw "Allocation calls were found in the telemetry hot path."
}

Write-Host "No dynamic-allocation calls found in the telemetry hot path."
