[CmdletBinding()]
param(
    [string]$Treeish = "HEAD",
    [switch]$Worktree,
    [switch]$UseWorktreeAttributes,
    [switch]$SkipValidation
)

Set-StrictMode -Version Latest
$ErrorActionPreference = "Stop"

$repositoryRoot = [System.IO.Path]::GetFullPath((Join-Path $PSScriptRoot ".."))
$workRoot = Join-Path $repositoryRoot ("build/source-archive-check-" + [guid]::NewGuid().ToString("N"))
$archivePath = Join-Path $workRoot "source.zip"
$extractRoot = Join-Path $workRoot "extracted"

try {
    [void](New-Item -ItemType Directory -Path $workRoot -Force)
    & git -C $repositoryRoot rev-parse --verify "$Treeish`^{tree}" *> $null
    if ($LASTEXITCODE -ne 0) { throw "Cannot resolve treeish '$Treeish'." }

    $useWorktreeSnapshot = $Worktree -or $UseWorktreeAttributes
    $archiveTreeish = $Treeish
    if ($useWorktreeSnapshot) {
        $temporaryIndex = Join-Path $workRoot "worktree.index"
        $previousIndex = $env:GIT_INDEX_FILE
        try {
            $env:GIT_INDEX_FILE = $temporaryIndex
            & git -C $repositoryRoot read-tree $Treeish
            if ($LASTEXITCODE -ne 0) { throw "Cannot initialize the temporary worktree index." }
            & git -C $repositoryRoot add -A -- .
            if ($LASTEXITCODE -ne 0) { throw "Cannot stage the complete worktree in the temporary index." }
            $archiveTreeish = (& git -C $repositoryRoot write-tree).Trim()
            if ($LASTEXITCODE -ne 0 -or -not $archiveTreeish) {
                throw "Cannot create the synthetic worktree tree."
            }
        }
        finally {
            $env:GIT_INDEX_FILE = $previousIndex
        }
    }

    $archiveArguments = @("archive", "--format=zip", "--output=$archivePath")
    if ($useWorktreeSnapshot) { $archiveArguments += "--worktree-attributes" }
    $archiveArguments += $archiveTreeish
    & git -C $repositoryRoot @archiveArguments
    if ($LASTEXITCODE -ne 0) { throw "git archive failed for '$archiveTreeish'." }

    Add-Type -AssemblyName System.IO.Compression.FileSystem
    $archive = [System.IO.Compression.ZipFile]::OpenRead($archivePath)
    try {
        $archiveFiles = @($archive.Entries |
            Where-Object { -not $_.FullName.EndsWith("/") } |
            ForEach-Object { $_.FullName.Replace("\", "/") } |
            Sort-Object -Unique)
    }
    finally {
        $archive.Dispose()
    }

    $trackedFiles = @(& git -C $repositoryRoot ls-tree -r --name-only $archiveTreeish |
        ForEach-Object { $_.Replace("\", "/") } |
        Sort-Object -Unique)
    if ($LASTEXITCODE -ne 0) { throw "Cannot enumerate files for '$archiveTreeish'." }

    $requiredRootFiles = @("CHANGELOG.md", "LICENSE", "README.md", "run_ci.bat")
    $missingRootFiles = @($requiredRootFiles | Where-Object { $_ -notin $archiveFiles })
    if ($missingRootFiles.Count -ne 0) {
        throw "Source archive is missing required root files: $($missingRootFiles -join ', ')"
    }

    $manifestCandidates = @("package.json", "pyproject.toml", "Cargo.toml", "library.json", "library.properties", "CMakeLists.txt") +
        @($trackedFiles | Where-Object { $_ -match '\.(sln|csproj)$' })
    if (@($manifestCandidates | Where-Object { $_ -in $archiveFiles }).Count -eq 0) {
        throw "Source archive contains no recognized build manifest."
    }

    $guideRoots = @("docsrc/user", "docs")
    foreach ($guide in @("GETTING_STARTED.md", "USAGE_GUIDE.md", "PROFILES.md", "GOTCHAS.md", "API_REFERENCE.md")) {
        if (@($guideRoots | ForEach-Object { "$_/$guide" } | Where-Object { $_ -in $archiveFiles }).Count -eq 0) {
            throw "Source archive is missing standard user guide '$guide'."
        }
    }

    $requiredTracked = @($trackedFiles | Where-Object {
        $_ -match '^(test|tests|\.github|docsrc/maintainer|internal_docs|scripts|tools)/' -or
        $_ -in @("AGENTS.md", "TODO.md", "release_check.bat", "run_ci.bat")
    })
    $missingTracked = @($requiredTracked | Where-Object { $_ -notin $archiveFiles })
    if ($missingTracked.Count -ne 0) {
        throw "Source archive omits tracked validation or maintainer material: $($missingTracked -join ', ')"
    }
    if (@($archiveFiles | Where-Object { $_ -match '^(test|tests)/' }).Count -eq 0) {
        throw "Source archive contains no repository tests."
    }

    $forbidden = @($archiveFiles | Where-Object {
        $_ -match '^(build|build_win|release-artifacts)/'
    })
    if ($forbidden.Count -ne 0) {
        throw "Source archive contains generated or release-output files: $($forbidden -join ', ')"
    }

    if (-not $SkipValidation) {
        Expand-Archive -LiteralPath $archivePath -DestinationPath $extractRoot
        Push-Location $extractRoot
        try {
            if ($env:OS -eq "Windows_NT") {
                & cmd.exe /d /c run_ci.bat --build-dir build\source-archive-validation
                if ($LASTEXITCODE -ne 0) { throw "run_ci.bat failed from the extracted source archive." }
            }
            else {
                & cmake -S . -B build/source-archive-validation -DCMAKE_CXX_STANDARD=17 -DCMAKE_CXX_EXTENSIONS=OFF
                if ($LASTEXITCODE -ne 0) { throw "CMake configure failed from the extracted source archive." }
                & cmake --build build/source-archive-validation
                if ($LASTEXITCODE -ne 0) { throw "CMake build failed from the extracted source archive." }
                & ctest --test-dir build/source-archive-validation --output-on-failure
                if ($LASTEXITCODE -ne 0) { throw "CTest failed from the extracted source archive." }
                & python scripts/check_markdown_links.py
                if ($LASTEXITCODE -ne 0) { throw "Markdown link check failed from the extracted source archive." }
                & python scripts/update_api_reference.py --check
                if ($LASTEXITCODE -ne 0) { throw "API reference check failed from the extracted source archive." }
            }
            & python scripts/check_platformio_package_consumers.py
            if ($LASTEXITCODE -ne 0) { throw "Packed PlatformIO consumer checks failed from the extracted source archive." }
        }
        finally {
            Pop-Location
        }
    }

    $sourceLabel = if ($useWorktreeSnapshot) { "worktree" } else { $Treeish }
    Write-Host "[OK] Source archive contract passed: source=$sourceLabel files=$($archiveFiles.Count) validation=$(-not $SkipValidation) packed-consumer=$(-not $SkipValidation)"
}
finally {
    if (Test-Path -LiteralPath $workRoot) {
        Remove-Item -LiteralPath $workRoot -Recurse -Force
    }
}
