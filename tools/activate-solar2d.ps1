# ////////////////////////////////////////////////////////////////////////////
# //
# // This file is part of the Solar2D game engine.
# // For overview and more information on licensing please refer to README.md 
# // Home page: https://github.com/coronalabs/corona
# // Contact: support@Solar2D.com
# //
# ////////////////////////////////////////////////////////////////////////////

param(
    [Parameter(Mandatory = $true)]
    [string]$InstallPath,

    [string]$SourcePath,

    [string]$Version,

    [switch]$CopyContent,

    [ValidateSet('User', 'Machine')]
    [string]$Scope = 'User'
)

$ErrorActionPreference = 'Stop'

function Resolve-Version {
    param(
        [string]$ExplicitVersion,
        [string]$RepoRoot
    )

    if ($ExplicitVersion) {
        return $ExplicitVersion
    }

    $versionFile = Join-Path $RepoRoot 'librtt/Core/Rtt_Version.h'
    if (-not (Test-Path $versionFile)) {
        return "local.$([DateTime]::Now.ToString('yyyyMMddHHmmss'))"
    }

    $content = Get-Content -Path $versionFile
    $yearMatch = $content | Select-String -Pattern '#define\s+Rtt_BUILD_YEAR\s+(\d+)' -AllMatches | Select-Object -First 1
    if (-not $yearMatch) {
        return "local.$([DateTime]::Now.ToString('yyyyMMddHHmmss'))"
    }
    $year = $yearMatch.Matches[0].Groups[1].Value

    $revMatch = $content | Select-String -Pattern '#define\s+Rtt_BUILD_REVISION\s+([^\s]+)' -AllMatches | Select-Object -First 1
    $revisionToken = $null
    if ($revMatch) {
        $revisionToken = $revMatch.Matches[0].Groups[1].Value
    }

    if (-not $revisionToken) {
        return "$year.local"
    }

    if ($revisionToken -match '^[0-9]+$') {
        return "$year.$revisionToken"
    }

    if ($revisionToken -eq 'Rtt_LOCAL_BUILD_REVISION') {
        $localMatch = $content | Select-String -Pattern '#define\s+Rtt_LOCAL_BUILD_REVISION\s+(\d+)' -AllMatches | Select-Object -First 1
        if ($localMatch) {
            return "$year.$($localMatch.Matches[0].Groups[1].Value)"
        }
    }

    return "$year.$revisionToken"
}

function Set-EnvVar {
    param(
        [string]$Name,
        [string]$Value,
        [System.EnvironmentVariableTarget]$Target
    )

    [Environment]::SetEnvironmentVariable($Name, $Value, $Target)
}

function Broadcast-EnvironmentChange {
    if (-not ('NativeMethods.User32' -as [type])) {
        Add-Type -Namespace NativeMethods -Name User32 -MemberDefinition @'
using System;
using System.Runtime.InteropServices;

public static class User32
{
    [DllImport("user32.dll", SetLastError = true, CharSet = CharSet.Auto)]
    public static extern IntPtr SendMessageTimeout(IntPtr hWnd, int Msg, UIntPtr wParam, string lParam, uint fuFlags, uint uTimeout, out UIntPtr lpdwResult);
}
'@
    }

    $HWND_BROADCAST = [IntPtr]0xffff
    $WM_SETTINGCHANGE = 0x001A
    $SMTO_ABORTIFHUNG = 0x0002
    [UIntPtr]$result = [UIntPtr]::Zero
    [void][NativeMethods.User32]::SendMessageTimeout($HWND_BROADCAST, $WM_SETTINGCHANGE, [UIntPtr]::Zero, "Environment", $SMTO_ABORTIFHUNG, 5000, [ref]$result)
}

$repoRoot = Split-Path -Parent $PSScriptRoot
if (-not $SourcePath) {
    $SourcePath = Join-Path $repoRoot 'platform/windows/bin/Corona'
}

$resolvedInstallPath = [System.IO.Path]::GetFullPath($InstallPath)
if (-not $resolvedInstallPath.EndsWith('\')) {
    $resolvedInstallPath += '\\'
}

$resolvedSourcePath = $null
if ($SourcePath) {
    $resolvedSourcePath = [System.IO.Path]::GetFullPath($SourcePath)
}

if ($CopyContent) {
    if (-not (Test-Path $resolvedSourcePath)) {
        throw "Source path '$resolvedSourcePath' does not exist."
    }

    if ($resolvedSourcePath.TrimEnd('\') -ieq $resolvedInstallPath.TrimEnd('\')) {
        Write-Verbose 'Source and install paths are the same; skipping copy.'
    } else {
        New-Item -ItemType Directory -Force -Path $resolvedInstallPath | Out-Null
        $robocopy = Get-Command robocopy.exe -ErrorAction SilentlyContinue
        if ($robocopy) {
            $arguments = @($resolvedSourcePath, $resolvedInstallPath, '/MIR', '/NFL', '/NDL', '/NJH', '/NJS', '/NC', '/NS', '/NP')
            & $robocopy.Path @arguments | Out-Null
            $robocopyExit = $LASTEXITCODE
            if ($robocopyExit -gt 7) {
                throw "robocopy failed with exit code $robocopyExit."
            }
        } else {
            Copy-Item -Path (Join-Path $resolvedSourcePath '*') -Destination $resolvedInstallPath -Recurse -Force
        }
    }
} else {
    if (-not (Test-Path $resolvedInstallPath)) {
        throw "Install path '$resolvedInstallPath' does not exist. Use -CopyContent to populate it from build output."
    }
}

$resolvedVersion = Resolve-Version -ExplicitVersion $Version -RepoRoot $repoRoot

$targetScope = [System.EnvironmentVariableTarget]::$Scope
Set-EnvVar -Name 'CORONA_PATH' -Value $resolvedInstallPath -Target $targetScope
Set-EnvVar -Name 'CORONA_SDK_PATH' -Value $resolvedInstallPath -Target $targetScope
$nativePath = Join-Path $resolvedInstallPath 'Native'
Set-EnvVar -Name 'CORONA_ROOT' -Value $nativePath -Target $targetScope
Set-EnvVar -Name 'SOLAR2D_INSTALL_PATH' -Value $resolvedInstallPath -Target $targetScope
if ($Version -or $resolvedVersion) {
    Set-EnvVar -Name 'SOLAR2D_VERSION' -Value $resolvedVersion -Target $targetScope
}

if ($Scope -eq 'Machine') {
    # Mirror to current user so new shells pick up the value without elevation.
    Set-EnvVar -Name 'CORONA_PATH' -Value $resolvedInstallPath -Target ([System.EnvironmentVariableTarget]::User)
    Set-EnvVar -Name 'CORONA_SDK_PATH' -Value $resolvedInstallPath -Target ([System.EnvironmentVariableTarget]::User)
    Set-EnvVar -Name 'CORONA_ROOT' -Value $nativePath -Target ([System.EnvironmentVariableTarget]::User)
    Set-EnvVar -Name 'SOLAR2D_INSTALL_PATH' -Value $resolvedInstallPath -Target ([System.EnvironmentVariableTarget]::User)
    if ($Version -or $resolvedVersion) {
        Set-EnvVar -Name 'SOLAR2D_VERSION' -Value $resolvedVersion -Target ([System.EnvironmentVariableTarget]::User)
    }
}

$registryRelative = 'Software\\Corona Labs\\Corona SDK\\Install'
$hkcuPath = "HKCU:$registryRelative"
New-Item -Path $hkcuPath -Force | Out-Null
Set-ItemProperty -Path $hkcuPath -Name Path -Value $resolvedInstallPath
Set-ItemProperty -Path $hkcuPath -Name Version -Value $resolvedVersion

if ($Scope -eq 'Machine') {
    $hklmPath = "HKLM:$registryRelative"
    try {
        New-Item -Path $hklmPath -Force | Out-Null
        Set-ItemProperty -Path $hklmPath -Name Path -Value $resolvedInstallPath
        Set-ItemProperty -Path $hklmPath -Name Version -Value $resolvedVersion
    } catch {
        Write-Warning "Failed to update HKLM registry entries: $_"
    }
}

Broadcast-EnvironmentChange

Write-Host "Solar2D install path set to $resolvedInstallPath"
Write-Host "Active version set to $resolvedVersion"
