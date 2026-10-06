#Requires -Version 5.1
[CmdletBinding()]
param([switch] $InstallExtensions)

try {
    . (Join-Path $PSScriptRoot 'common.ps1')
    Assert-CliVersion
    Assert-Sources

    # Preflight ALL conflicts before installing anything. Do not replace another
    # project's global core or libraries; the user can choose a separate CLI profile.
    $core = @(Get-InstalledCore)
    if ($core.Count -gt 0 -and $core[0].installed_version -ne $EnvironmentLock.core.version) {
        throw "Core conflict: $($EnvironmentLock.core.id) $($core[0].installed_version). Expected $($EnvironmentLock.core.version). No packages changed."
    }
    $installed = @(Get-InstalledLibraries)
    foreach ($required in $EnvironmentLock.libraries) {
        $found = @($installed | Where-Object { $_.name -eq $required.name })
        if (@($found | Where-Object { $_.version -ne $required.version }).Count -gt 0) {
            throw "Library conflict: $($required.name). Expected $($required.version). No packages changed."
        }
    }

    if ($core.Count -eq 0) {
        Invoke-Arduino @('core', 'update-index')
        Invoke-Arduino @('core', 'install', "$($EnvironmentLock.core.id)@$($EnvironmentLock.core.version)")
    } else {
        Write-Host "[PASS] Core already installed: $($EnvironmentLock.core.id)@$($EnvironmentLock.core.version)"
    }

    $missing = @($EnvironmentLock.libraries | Where-Object { $requiredName = $_.name; @($installed | Where-Object { $_.name -eq $requiredName }).Count -eq 0 })
    if ($missing.Count -gt 0) {
        Invoke-Arduino @('lib', 'update-index')
        foreach ($required in $missing) {
            # All dependencies are explicit in the lock; do not pull latest versions.
            Invoke-Arduino @('lib', 'install', '--no-deps', "$($required.name)@$($required.version)")
        }
    } else {
        Write-Host '[PASS] All pinned libraries already installed'
    }

    if ($InstallExtensions) {
        Require-Command 'code'
        $recommendations = Get-Content -LiteralPath (Join-Path $RepoRoot '.vscode/extensions.json') -Raw | ConvertFrom-Json
        $existingExtensions = @(& code --list-extensions)
        if ($LASTEXITCODE -ne 0) { throw 'Cannot list VS Code extensions.' }
        foreach ($extension in $recommendations.recommendations) {
            if ($existingExtensions -notcontains $extension) {
                & code --install-extension $extension
                if ($LASTEXITCODE -ne 0) { throw "VS Code extension installation failed: $extension" }
            } else {
                Write-Host "[PASS] Extension already installed: $extension"
            }
        }
    }

    # Verification includes the clean compile and checked native exit codes.
    & (Join-Path $PSScriptRoot 'verify_environment.ps1')
    if ($LASTEXITCODE -ne 0) { throw 'Environment verification failed.' }
    Write-Host '[PASS] Setup complete. No hardware upload was performed.'
    exit 0
} catch {
    Write-Host "[FAIL] $($_.Exception.Message)" -ForegroundColor Red
    exit 1
}
