# Shared helpers for Windows PowerShell 5.1 and PowerShell 7.
Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'
$RepoRoot = Split-Path -Parent $PSScriptRoot
$EnvironmentLock = Get-Content -LiteralPath (Join-Path $PSScriptRoot 'environment.lock.json') -Raw | ConvertFrom-Json

function Initialize-ArduinoEnvironment {
    # Resolve from the repository, regardless of the caller's working directory.
    # Both setup and verify always use this profile, leaving other projects alone.
    $localArduinoRoot = Join-Path $RepoRoot '.arduino-local'
    $env:ARDUINO_DIRECTORIES_DATA = Join-Path $localArduinoRoot 'data'
    $env:ARDUINO_DIRECTORIES_DOWNLOADS = Join-Path $localArduinoRoot 'downloads'
    $env:ARDUINO_DIRECTORIES_USER = Join-Path $localArduinoRoot 'user'

    # Also support the user ZIP installation before the shell's PATH is refreshed.
    $cliDirectory = Join-Path $env:LOCALAPPDATA "ArduinoCLI\$($EnvironmentLock.arduinoCli)"
    if (Test-Path -LiteralPath (Join-Path $cliDirectory 'arduino-cli.exe') -PathType Leaf) {
        $otherPaths = @($env:Path -split ';' | Where-Object { $_ -and $_ -ne $cliDirectory })
        $env:Path = (@($cliDirectory) + $otherPaths) -join ';'
    }
    Write-Host "[PASS] Repository-local Arduino environment: $localArduinoRoot"
}

function Require-Command([string] $Name) {
    if (-not (Get-Command $Name -ErrorAction SilentlyContinue)) {
        throw "Missing command: $Name. See docs/NEW_PC_SETUP.md and reopen PowerShell after installation."
    }
}

function Invoke-Arduino([string[]] $CliArguments) {
    & arduino-cli @CliArguments
    if ($LASTEXITCODE -ne 0) {
        throw "arduino-cli failed (exit $LASTEXITCODE): $($CliArguments -join ' ')"
    }
}

function Get-ArduinoJson([string[]] $CliArguments) {
    $result = Invoke-Arduino ($CliArguments + @('--format', 'json'))
    return (($result -join [Environment]::NewLine) | ConvertFrom-Json)
}

function Assert-CliVersion {
    Require-Command 'arduino-cli'
    $versionText = (Invoke-Arduino @('version')) -join ' '
    $match = [regex]::Match($versionText, 'Version:\s*(\S+)')
    if (-not $match.Success -or $match.Groups[1].Value -ne $EnvironmentLock.arduinoCli) {
        throw "Arduino CLI mismatch. Expected $($EnvironmentLock.arduinoCli); found: $versionText"
    }
    Write-Host "[PASS] $versionText"
}

function Get-InstalledCore {
    $data = Get-ArduinoJson @('core', 'list')
    return @($data.platforms | Where-Object { $_.id -eq $EnvironmentLock.core.id })
}

function Get-InstalledLibraries {
    $data = Get-ArduinoJson @('lib', 'list')
    return @($data.installed_libraries | ForEach-Object { $_.library })
}

function Assert-Sources {
    foreach ($relativePath in @("$($EnvironmentLock.sketch)/$($EnvironmentLock.sketch).ino", "$($EnvironmentLock.sketch)/config.h")) {
        if (-not (Test-Path -LiteralPath (Join-Path $RepoRoot $relativePath) -PathType Leaf)) {
            throw "Missing source: $relativePath"
        }
        Write-Host "[PASS] Source: $relativePath"
    }
}

function Build-Mega {
    Assert-Sources
    # Use a fresh directory every time; --clean also rebuilds the core cache.
    # Keep artifacts for explicit upload; never upload or open a serial port here.
    $buildParent = Join-Path $RepoRoot 'build'
    $buildDirectory = Join-Path $buildParent ('verify-' + [guid]::NewGuid().ToString('N'))
    New-Item -ItemType Directory -Path $buildDirectory -Force | Out-Null
    Invoke-Arduino @('compile', '--clean', '--fqbn', $EnvironmentLock.fqbn, '--build-path', $buildDirectory, (Join-Path $RepoRoot $EnvironmentLock.sketch))
    Write-Host "[PASS] Clean Mega 2560 compile: $buildDirectory"
}
