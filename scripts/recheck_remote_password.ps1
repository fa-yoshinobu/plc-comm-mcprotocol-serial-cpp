[CmdletBinding()]
param(
    [string]$CliPath = ".\build\manual\mcprotocol_cli.exe",
    [Parameter(Mandatory = $true)]
    [string]$Device,
    [Parameter(Mandatory = $true)]
    [int]$Baud,
    [Parameter(Mandatory = $true)]
    [int]$DataBits,
    [Parameter(Mandatory = $true)]
    [ValidateSet("N", "E", "O")]
    [string]$Parity,
    [Parameter(Mandatory = $true)]
    [int]$StopBits,
    [Parameter(Mandatory = $true)]
    [ValidateSet("none", "rts-cts")]
    [string]$HardwareFlow,
    [string]$Frame = "c4-binary",
    [string]$PlcProfile = "melsec:iq-r",
    [Parameter(Mandatory = $true)]
    [ValidateSet("host", "multidrop")]
    [string]$Route,
    [Nullable[int]]$Station,
    [Nullable[int]]$Network,
    [string]$PcTarget,
    [string]$ModuleTarget,
    [ValidateSet("standard", "mn")]
    [string]$Topology,
    [Nullable[int]]$SelfStation,
    [Parameter(Mandatory = $true)]
    [ValidateSet("on", "off")]
    [string]$SumCheck,
    [int]$ResponseTimeoutMs = 3000,
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
if ($ResponseTimeoutMs -le 0) {
    throw "ResponseTimeoutMs must be in range 1..2147483647."
}
if ($InterByteTimeoutMs -le 0) {
    throw "InterByteTimeoutMs must be in range 1..2147483647."
}

if ($Route -eq "multidrop" -and $null -eq $Station) {
    throw "Station is required when -Route multidrop is selected."
}
if ($Route -eq "host" -and $null -ne $Station) {
    throw "Station must not be specified when -Route host is selected."
}
if ($Route -eq "host" -and $null -ne $Network) {
    throw "Network must not be specified when -Route host is selected."
}
if ($Route -eq "host" -and -not [string]::IsNullOrEmpty($PcTarget)) {
    throw "PcTarget must not be specified when -Route host is selected."
}
if ($Route -eq "host" -and -not [string]::IsNullOrEmpty($ModuleTarget)) {
    throw "ModuleTarget must not be specified when -Route host is selected."
}
if ($Route -eq "multidrop" -and $Frame -match '^c[34]-' -and $null -eq $Network) {
    throw "Network is required for 3C/4C multidrop routes."
}
if ($Route -eq "multidrop" -and $Frame -match '^c[12]-' -and $null -ne $Network) {
    throw "Network must not be specified for 1C/2C multidrop routes."
}
if ($Route -eq "multidrop" -and $Frame -match '^c[34]-' -and [string]::IsNullOrEmpty($PcTarget)) {
    throw "PcTarget is required for 3C/4C non-host routes."
}
if ($Route -eq "multidrop" -and $Frame -match '^c[12]-' -and -not [string]::IsNullOrEmpty($PcTarget)) {
    throw "PcTarget must not be specified for 1C/2C routes."
}
if ($Route -eq "multidrop" -and $Frame -match '^c4-' -and [string]::IsNullOrEmpty($ModuleTarget)) {
    throw "ModuleTarget is required for a 4C multidrop route."
}
if ($Route -eq "multidrop" -and $Frame -notmatch '^c4-' -and -not [string]::IsNullOrEmpty($ModuleTarget)) {
    throw "ModuleTarget must not be specified outside a 4C multidrop route."
}
if ($Route -eq "multidrop" -and $Frame -match '^c[234]-') {
    if ([string]::IsNullOrEmpty($Topology)) {
        throw "Topology is required for 2C/3C/4C multidrop routes."
    }
    if ($Topology -eq "standard" -and $null -ne $SelfStation) {
        throw "SelfStation must not be specified for standard topology."
    }
    if ($Topology -eq "mn" -and $null -eq $SelfStation) {
        throw "SelfStation is required for mn topology."
    }
    if ($null -ne $SelfStation -and ($SelfStation -lt 0 -or $SelfStation -gt 31)) {
        throw "SelfStation must be in range 0..31."
    }
} elseif (-not [string]::IsNullOrEmpty($Topology) -or $null -ne $SelfStation) {
    throw "Topology and SelfStation must not be specified for this route/frame."
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
    "--hardware-flow", $HardwareFlow,
    "--frame", $Frame,
    "--plc-profile", $PlcProfile,
    "--route", $Route,
    "--sum-check", $SumCheck,
    "--response-timeout-ms", "$ResponseTimeoutMs",
    "--inter-byte-timeout-ms", "$InterByteTimeoutMs",
    "--dump-frames", "on"
)
if ($null -ne $Station) {
    $script:CommonArgs += @("--station", "$Station")
}
if ($null -ne $Network) {
    $script:CommonArgs += @("--network", "$Network")
}
if (-not [string]::IsNullOrEmpty($PcTarget)) {
    $script:CommonArgs += @("--pc-target", $PcTarget)
}
if (-not [string]::IsNullOrEmpty($ModuleTarget)) {
    $script:CommonArgs += @("--module-target", $ModuleTarget)
}
if (-not [string]::IsNullOrEmpty($Topology)) {
    $script:CommonArgs += @("--topology", $Topology)
}
if ($null -ne $SelfStation) {
    $script:CommonArgs += @("--self-station", "$SelfStation")
}
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
Write-LogLine "route=$Route station=$Station network=$Network pc_target=$PcTarget module_target=$ModuleTarget"
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
