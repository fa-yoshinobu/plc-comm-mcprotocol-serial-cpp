[CmdletBinding()]
param(
    [string]$CliPath = ".\build\manual\mcprotocol_cli.exe",
    [string]$Device = "COM3",
    [int]$Baud = 28800,
    [int]$DataBits = 8,
    [ValidateSet("N", "E", "O")]
    [string]$Parity = "E",
    [int]$StopBits = 2,
    [string]$Frame = "c4-binary",
    [string]$PlcProfile = "melsec:iq-r",
    [int]$Station = 0,
    [ValidateSet("on", "off")]
    [string]$SumCheck = "on",
    [int]$ResponseTimeoutMs = 5000,
    [int]$InterByteTimeoutMs = 250,
    [string]$Password = "",
    [switch]$AllowRemotePasswordCommands,
    [string]$LogDirectory = ".\logs"
)

$ErrorActionPreference = "Stop"

$resolvedCli = Resolve-Path -LiteralPath $CliPath -ErrorAction SilentlyContinue
if ($null -eq $resolvedCli) {
    throw "CLI executable not found: $CliPath"
}

if ($AllowRemotePasswordCommands -and [string]::IsNullOrEmpty($Password)) {
    throw "Password is required when -AllowRemotePasswordCommands is set."
}

if (-not (Test-Path -LiteralPath $LogDirectory)) {
    New-Item -ItemType Directory -Path $LogDirectory | Out-Null
}

$timestamp = Get-Date -Format "yyyyMMdd_HHmmss"
$script:LogPath = Join-Path -Path $LogDirectory -ChildPath "remote_password_recheck_$timestamp.log"
$script:CliPathResolved = $resolvedCli.ProviderPath
$script:PasswordValue = $Password
$script:CommonArgs = @(
    "--device", $Device,
    "--baud", "$Baud",
    "--data-bits", "$DataBits",
    "--parity", $Parity,
    "--stop-bits", "$StopBits",
    "--frame", $Frame,
    "--plc-profile", $PlcProfile,
    "--station", "$Station",
    "--sum-check", $SumCheck,
    "--response-timeout-ms", "$ResponseTimeoutMs",
    "--inter-byte-timeout-ms", "$InterByteTimeoutMs",
    "--dump-frames", "on"
)

function Write-LogLine {
    param([string]$Line)
    $Line | Tee-Object -FilePath $script:LogPath -Append
}

function Format-CommandLine {
    param(
        [string[]]$CommandArgs,
        [switch]$Sensitive
    )

    $displayArgs = foreach ($item in $CommandArgs) {
        if ($Sensitive -and $item -eq $script:PasswordValue) {
            "<password>"
        } else {
            $item
        }
    }

    return "$script:CliPathResolved $($script:CommonArgs + $displayArgs -join ' ')"
}

function Invoke-CliStep {
    param(
        [string]$Name,
        [string[]]$CommandArgs,
        [switch]$Sensitive
    )

    Write-LogLine ""
    Write-LogLine "### $Name"
    Write-LogLine (Format-CommandLine -CommandArgs $CommandArgs -Sensitive:$Sensitive)

    $allArgs = @($script:CommonArgs + $CommandArgs)
    $output = & $script:CliPathResolved @allArgs 2>&1
    $exitCode = $LASTEXITCODE

    foreach ($line in $output) {
        Write-LogLine ($line.ToString())
    }
    Write-LogLine "exit-code=$exitCode"

    return [pscustomobject]@{
        Name = $Name
        ExitCode = $exitCode
    }
}

Write-Warning "This log includes raw TX/RX frames. Remote password bytes are visible in hex when unlock/lock are run."

Write-LogLine "# Remote password 1630/1631 recheck"
Write-LogLine "timestamp=$timestamp"
Write-LogLine "device=$Device"
Write-LogLine "serial=$Baud/${DataBits}${Parity}${StopBits}"
Write-LogLine "frame=$Frame"
Write-LogLine "plc-profile=$PlcProfile"
Write-LogLine "station=$Station"
Write-LogLine "sum-check=$SumCheck"
Write-LogLine "response-timeout-ms=$ResponseTimeoutMs"
Write-LogLine "inter-byte-timeout-ms=$InterByteTimeoutMs"
if ([string]::IsNullOrEmpty($Password)) {
    Write-LogLine "password-length=<not provided>"
} else {
    Write-LogLine "password-length=$($Password.Length)"
}
Write-LogLine "allow-remote-password-commands=$AllowRemotePasswordCommands"

$results = @()
$results += Invoke-CliStep -Name "Sanity before: cpu-model" -CommandArgs @("cpu-model")
$results += Invoke-CliStep -Name "Sanity before: read-words D0 1" -CommandArgs @("read-words", "D0", "1")

if (-not $AllowRemotePasswordCommands) {
    Write-LogLine ""
    Write-LogLine "Remote password unlock/lock were not run."
    Write-LogLine "Re-run with -AllowRemotePasswordCommands only after the target-side remote password configuration is known."
    Write-Host "Read-only sanity log written to $script:LogPath"
    exit 0
}

$results += Invoke-CliStep -Name "Unlock remote password: 1630" -CommandArgs @("unlock", $Password) -Sensitive
$results += Invoke-CliStep -Name "Sanity after unlock: read-words D0 1" -CommandArgs @("read-words", "D0", "1")
$results += Invoke-CliStep -Name "Lock remote password: 1631" -CommandArgs @("lock", $Password) -Sensitive
$results += Invoke-CliStep -Name "Sanity after lock: cpu-model" -CommandArgs @("cpu-model")
$results += Invoke-CliStep -Name "Sanity after lock: read-words D0 1" -CommandArgs @("read-words", "D0", "1")

Write-LogLine ""
Write-LogLine "## Result summary"
foreach ($result in $results) {
    Write-LogLine "$($result.Name): exit-code=$($result.ExitCode)"
}

$failed = @($results | Where-Object { $_.ExitCode -ne 0 })
Write-Host "Remote password recheck log written to $script:LogPath"
if ($failed.Count -gt 0) {
    exit 1
}

exit 0
