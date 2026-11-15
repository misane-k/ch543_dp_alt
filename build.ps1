param(
    [string]$KeilUv4 = 'C:\software\Keil_v5\UV4\UV4.exe',
    [string]$Project = (Join-Path $PSScriptRoot 'main.uvproj')
)

$ErrorActionPreference = 'Stop'
$logDirectory = Join-Path $PSScriptRoot 'build_logs'
$timestamp = Get-Date -Format 'yyyyMMdd_HHmmss'
$logFile = Join-Path $logDirectory "build_$timestamp.log"
$buildLog = Join-Path $PSScriptRoot 'Objects\main.build_log.htm'

New-Item -ItemType Directory -Force -Path $logDirectory | Out-Null

if( -not (Test-Path $KeilUv4) ) {
    throw "Keil UV4 not found: $KeilUv4"
}
if( -not (Test-Path $Project) ) {
    throw "Keil project not found: $Project"
}

& $KeilUv4 -b $Project -j0 2>&1 | Tee-Object -FilePath $logFile

if( Test-Path $buildLog ) {
    $html = Get-Content -Raw -Path $buildLog
    $text = [System.Net.WebUtility]::HtmlDecode(($html -replace '<[^>]+>', ' '))
    $text | Set-Content -Encoding UTF8 -Path $logFile
    $result = [regex]::Match($text, '"\.\\Objects\\main"\s+-\s+(\d+) Error\(s\),\s+(\d+) Warning\(s\)')
    if( $result.Success ) {
        $errors = [int]$result.Groups[1].Value
        $warnings = [int]$result.Groups[2].Value
        Write-Host "Build result: $errors error(s), $warnings warning(s)"
        Write-Host "Log: $logFile"
        if( $errors -ne 0 ) { exit 1 }
        exit 0
    }
    Write-Host "Build log format not recognized. Log: $logFile"
    exit 2
}

throw "Keil build log not found: $buildLog"
