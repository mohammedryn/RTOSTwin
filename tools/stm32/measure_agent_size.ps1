param(
    [Parameter(Mandatory = $true)]
    [string]$MapFile
)

$ErrorActionPreference = "Stop"

if (-not (Test-Path -LiteralPath $MapFile)) {
    throw "Map file not found: $MapFile"
}

$agentObjectNames = @(
    'telemetry_agent.o',
    'measurement.o',
    'snapshot.o',
    'encoder.o',
    'framer.o',
    'transport.o',
    'profiler.o',
    'hooks.o',
    'dwt.o',
    'uart_dma.o'
)

$sectionTotals = @{
    data = 0
    bss  = 0
}

$matchedRows = New-Object System.Collections.Generic.List[object]
$seenEntries = New-Object 'System.Collections.Generic.HashSet[string]'
$mapLines = Get-Content -Path $MapFile

function Add-AgentSectionMatch {
    param(
        [string]$SectionName,
        [int]$SizeBytes,
        [string]$ObjectRef
    )

    foreach ($objectName in $agentObjectNames) {
        if ($ObjectRef -like "*$objectName*") {
            $entryKey = "$SectionName|$SizeBytes|$ObjectRef"
            if ($seenEntries.Add($entryKey)) {
                $sectionTotals[$SectionName] += $SizeBytes
                $matchedRows.Add([pscustomobject]@{
                    Section = $SectionName
                    SizeBytes = $SizeBytes
                    Object = $ObjectRef
                }) | Out-Null
            }
            break
        }
    }
}

for ($i = 0; $i -lt $mapLines.Count; $i++) {
    $line = $mapLines[$i]

    if ($line -match '^\s*\.(data|bss)(\.[^\s]+)?\s+0x[0-9A-Fa-f]+\s+0x([0-9A-Fa-f]+)\s+(.+)$') {
        $sectionName = $matches[1]
        $sizeBytes = [Convert]::ToInt32($matches[3], 16)
        $objectRef = $matches[4].Trim()
        Add-AgentSectionMatch -SectionName $sectionName -SizeBytes $sizeBytes -ObjectRef $objectRef
        continue
    }

    if ($line -match '^\s*\.(data|bss)(\.[^\s]+)?\s*$') {
        $sectionName = $matches[1]
        if ($i + 1 -lt $mapLines.Count) {
            $nextLine = $mapLines[$i + 1]
            if ($nextLine -match '^\s*0x[0-9A-Fa-f]+\s+0x([0-9A-Fa-f]+)\s+(.+)$') {
                $sizeBytes = [Convert]::ToInt32($matches[1], 16)
                $objectRef = $matches[2].Trim()
                Add-AgentSectionMatch -SectionName $sectionName -SizeBytes $sizeBytes -ObjectRef $objectRef
            }
        }
    }
}

$total = $sectionTotals.data + $sectionTotals.bss

Write-Host "Agent .data bytes: $($sectionTotals.data)"
Write-Host "Agent .bss bytes:  $($sectionTotals.bss)"
Write-Host "Agent static RAM total bytes: $total"

$matchedRows | Sort-Object Section, Object | Format-Table -AutoSize
