Param(
    [Parameter(Mandatory=$false)]
    [String] $qmodName="",

    [Parameter(Mandatory=$false)]
    [Switch] $help
)

if ($help -eq $true) {
    Write-Output "`"createqmod`" - Creates a .qmod file with your compiled libraries and mod.json."
    Write-Output "`n-- Arguments --`n"

    Write-Output "-QmodName `t The file name of your qmod"

    exit
}

$repoRoot = Resolve-Path (Join-Path $PSScriptRoot "..")
Push-Location $repoRoot
try {
    & $PSScriptRoot/validate-modjson.ps1
    if ($LASTEXITCODE -ne 0) {
        exit $LASTEXITCODE
    }

    $mod = Join-Path $repoRoot "mod.json"
    $modJson = Get-Content $mod -Raw | ConvertFrom-Json

    if ($qmodName -eq "") {
        $qmodName = $modJson.name
    }

    $stagingDir = Join-Path $repoRoot ("qmod-staging-" + [Guid]::NewGuid().ToString("N"))
    New-Item -ItemType Directory -Force -Path $stagingDir | Out-Null

    function Copy-ToStaging([string]$sourcePath, [string]$destRelative) {
        $destPath = Join-Path $stagingDir $destRelative
        $destDir = Split-Path -Parent $destPath
        if ($destDir -and -not (Test-Path $destDir)) {
            New-Item -ItemType Directory -Force -Path $destDir | Out-Null
        }
        Copy-Item $sourcePath $destPath -Force
    }

    Copy-ToStaging $mod "mod.json"

    $cover = Join-Path $repoRoot $modJson.coverImage
    if ((-not ([string]::IsNullOrEmpty($modJson.coverImage))) -and (Test-Path $cover)) {
        Copy-ToStaging $cover $modJson.coverImage
    }

    foreach ($modFile in $modJson.modFiles) {
        $path = Join-Path $repoRoot ("build/" + $modFile)
        if (-not (Test-Path $path)) {
            $path = Join-Path $repoRoot ("extern/libs/" + $modFile)
        }
        if (-not (Test-Path $path)) {
            Write-Output "Error: could not find dependency: $path"
            exit 1
        }
        Copy-ToStaging $path $modFile
    }

    foreach ($modFile in $modJson.lateModFiles) {
        $path = Join-Path $repoRoot ("build/" + $modFile)
        if (-not (Test-Path $path)) {
            $path = Join-Path $repoRoot ("extern/libs/" + $modFile)
        }
        if (-not (Test-Path $path)) {
            Write-Output "Error: could not find dependency: $path"
            exit 1
        }
        Copy-ToStaging $path $modFile
    }

    foreach ($lib in $modJson.libraryFiles) {
        $path = Join-Path $repoRoot ("build/" + $lib)
        if (-not (Test-Path $path)) {
            $path = Join-Path $repoRoot ("extern/libs/" + $lib)
        }
        if (-not (Test-Path $path)) {
            Write-Output "Error: could not find dependency: $path"
            exit 1
        }
        Copy-ToStaging $path $lib
    }

    if ($modJson.fileCopies) {
        foreach ($copy in $modJson.fileCopies) {
            $fileName = $copy.name
            if ([string]::IsNullOrEmpty($fileName)) {
                continue
            }
            # Try to find the file in known locations
            $sourcePath = Join-Path $repoRoot $fileName
            if (-not (Test-Path $sourcePath)) {
                # Check kakasi dictionary location
                $kakasiPath = Join-Path $repoRoot ("third-party/kakasi/share/kakasi/" + $fileName)
                if (Test-Path $kakasiPath) {
                    $sourcePath = $kakasiPath
                }
            }
            if (-not (Test-Path $sourcePath)) {
                Write-Output "Error: could not find fileCopy source: $fileName"
                exit 1
            }
            Copy-ToStaging $sourcePath $fileName
        }
    }

    $zip = Join-Path $repoRoot ($qmodName + ".zip")
    $qmod = Join-Path $repoRoot ($qmodName + ".qmod")

    Compress-Archive -Path (Join-Path $stagingDir "*") -DestinationPath $zip -Force
    Move-Item $zip $qmod -Force
    Remove-Item $stagingDir -Recurse -Force
}
finally {
    Pop-Location
}
