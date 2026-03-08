$Action = $Args[0]
$RemainingArgs = $Args | Select-Object -Skip 1
$ExeName = "mgui-demo.exe"

if ($null -eq $Action) {
    $Action = "build"
}

switch ($Action) {
    "build" {
        Write-Host "Building mgui demo..." -ForegroundColor Cyan
        zig build $RemainingArgs
    }
    "run" {
        Write-Host "Running mgui demo..." -ForegroundColor Cyan
        zig build run -- $RemainingArgs
    }
    "release" {
        $Version = if ($RemainingArgs.Count -gt 0) { $RemainingArgs[0] } else { "0.1.0" }
        $ReleaseDir = "mgui-$Version-win64"
        $ZipFile = "$ReleaseDir.zip"

        Write-Host "Building release (ReleaseFast)..." -ForegroundColor Green
        zig build -Doptimize=ReleaseFast

        Write-Host "Packaging $ZipFile..." -ForegroundColor Cyan
        if (Test-Path $ReleaseDir) {
            Remove-Item -Path $ReleaseDir -Recurse -Force
        }
        New-Item -ItemType Directory -Path $ReleaseDir | Out-Null

        Copy-Item -Path "zig-out\bin\$ExeName" -Destination "$ReleaseDir\$ExeName"
        Copy-Item -Path "README.md" -Destination "$ReleaseDir\README.md"

        if (Test-Path $ZipFile) {
            Remove-Item -Path $ZipFile -Force
        }
        Compress-Archive -Path $ReleaseDir -DestinationPath $ZipFile
        Remove-Item -Path $ReleaseDir -Recurse -Force

        Write-Host "Release package created: $ZipFile" -ForegroundColor Green
    }
    "clean" {
        Write-Host "Cleaning build artifacts..." -ForegroundColor Yellow
        Remove-Item -Path "zig-out" -Recurse -Force -ErrorAction SilentlyContinue
        Remove-Item -Path ".zig-cache" -Recurse -Force -ErrorAction SilentlyContinue
    }
    default {
        Write-Host "Unknown action: $Action" -ForegroundColor Red
        Write-Host "Usage: ./scripts/build.ps1 [build|run|release|clean] [release_version]"
    }
}
