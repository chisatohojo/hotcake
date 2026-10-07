#Requires -Version 5.1
[CmdletBinding()]
param()

try {
    . (Join-Path $PSScriptRoot 'common.ps1')
    Initialize-ArduinoEnvironment
    Require-Command 'git'
    & git --version
    if ($LASTEXITCODE -ne 0) { throw 'git --version failed.' }
    Write-Host '[PASS] Git available'
    Assert-CliVersion

    $core = @(Get-InstalledCore)
    if ($core.Count -ne 1 -or $core[0].installed_version -ne $EnvironmentLock.core.version) {
        throw "Expected core $($EnvironmentLock.core.id)@$($EnvironmentLock.core.version). Run scripts/setup_windows.ps1."
    }
    Write-Host "[PASS] $($EnvironmentLock.core.id)@$($EnvironmentLock.core.version)"

    $installed = @(Get-InstalledLibraries)
    foreach ($required in $EnvironmentLock.libraries) {
        $found = @($installed | Where-Object { $_.name -eq $required.name })
        if ($found.Count -eq 0 -or @($found | Where-Object { $_.version -ne $required.version }).Count -gt 0) {
            throw "Expected library $($required.name)@$($required.version). Run scripts/setup_windows.ps1."
        }
        Write-Host "[PASS] $($required.name)@$($required.version)"
    }
    Build-Mega
    Write-Host '[PASS] Environment verified. Hardware behavior is not verified by compilation.'
    exit 0
} catch {
    Write-Host "[FAIL] $($_.Exception.Message)" -ForegroundColor Red
    exit 1
}
