$DefaultAccountName = "theSLGjenkins"
$ProjectSlug = "runtimegraphicsinfo"
$RuntimesPath = "$PSScriptRoot\..\Runtime\Plugins"

function BuildUtils-DownloadAppVeyorRuntimes([string]$Token = "", [string]$AccountName = "", [string]$BuildId = "") {
    $apiUrl = 'https://ci.appveyor.com/api'

    if ($Token -eq "")
    {
        $tokenFilePath = "$HOME/.appveyor/token"
        if (Test-Path $tokenFilePath)
        {
            $Token = Get-Content -Path $tokenFilePath
        }
        else
        {
            Write-Host "AppVeyor token not found";
            Exit 1;
        }
    }

    if ($AccountName -eq "")
    {
        $AccountName = $DefaultAccountName
    }

    $token = $Token
    $headers = @{
        "Authorization" = "Bearer $token"
        "Content-type" = "application/json"
    }

    if ($BuildId -eq "")
    {
        # get project with last build details
        $project = Invoke-RestMethod -Method Get -Uri "$apiUrl/projects/$AccountName/$ProjectSlug" -Headers $headers
    }
    else
    {
        # get project with specific build details
        $project = Invoke-RestMethod -Method Get -Uri "$apiUrl/projects/$AccountName/$ProjectSlug/build/$BuildId" -Headers $headers
    }

    # download artifacts for all jobs in the build
    foreach ($job in $project.build.jobs)
    {
        $jobId = $job.jobId

        # get job artifacts (just to see what we've got)
        $artifacts = Invoke-RestMethod -Method Get -Uri "$apiUrl/buildjobs/$jobId/artifacts" -Headers $headers
        foreach ($artifact in $artifacts)
        {
            $artifactFileName = $artifact.fileName
            $libName = Split-Path -Path $artifactFileName -Leaf
            $runtime = switch ($job.name)
            {
                "Image: Visual Studio 2019; Platform: x86" { "x86" }
                "Image: Visual Studio 2019; Platform: x64" { "x86_64" }
                "Image: Ubuntu; Platform: x64" { "x86_64" }
                "Image: macOS; Platform: x64" { "x86_64" }
            }
            $artifactUri = "$apiUrl/buildjobs/$jobId/artifacts/$artifactFileName"
            $localArtifactDir = "$RuntimesPath/$runtime"
            [void](New-Item -ItemType Directory -Force -Path $localArtifactDir)
            $localArtifactPath = "$localArtifactDir/$libName"
            Write-Host "$artifactUri -> $localArtifactPath"
            Invoke-RestMethod -Method Get -Uri "$apiUrl/buildjobs/$jobId/artifacts/$artifactFileName" -OutFile $localArtifactPath -Headers $headers
        }
    }
}
