
$Compiler = ".\cm.exe"
$TestDir = ".\tests"
$Config = "config/C"

if (-not (Test-Path $Compiler)) {
    Write-Error "Compiler not found: $Compiler"
    exit 1
}

if (-not (Test-Path $TestDir -PathType Container)) {
    Write-Error "Test directory not found: $TestDir"
    exit 1
}

$Total = 0
$Failed = 0

Get-ChildItem -Path $TestDir -Recurse -File -Filter "*.c" |
    ForEach-Object {
        $File = $_.FullName
        $Output = [System.IO.Path]::ChangeExtension($File, ".txt")

        Write-Host "Running: $File"

        try {
            & $Compiler $File $Config > $Output

            if ($LASTEXITCODE -ne 0) {
                Write-Host "  FAILED (exit code $LASTEXITCODE)" -ForegroundColor Red
                $Failed++
            }
            else {
                Write-Host "  Output: $Output" -ForegroundColor Green
            }
        }
        catch {
            Write-Host "  ERROR: $_" -ForegroundColor Red
            $Failed++
        }

        $Total++
    }

Write-Host "`nFinished."
Write-Host "Total tests: $Total"
Write-Host "Failed tests: $Failed"

if ($Failed -gt 0) {
    exit 1
}
