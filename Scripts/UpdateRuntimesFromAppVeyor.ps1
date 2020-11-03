param([string]$Token = "", [string]$AccountName = "mirasrael", [string]$BuildId = "", [switch]$Help = $false)

if ($Help)
{
    Write-Host "USAGE: powershell ./UpdateRuntimeFromAppVeyor.ps1 [-Token <Token>] [-AccountName <AccountName>] [-BuildId <BuildId>] [-Help]"
    Exit 0
}

$ErrorActionPreference = "Stop"

. $PSScriptRoot/BuildUtils.ps1

BuildUtils-DownloadAppVeyorRuntimes -Token $Token -AccountName $AccountName -BuildId $BuildId