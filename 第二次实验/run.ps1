# Lexical Analyzer - Mandatory Functions Demo
# Usage: .\run.ps1

Write-Host "========================================" -ForegroundColor Cyan
Write-Host "  Lexical Analyzer - Mandatory Demo" -ForegroundColor Cyan
Write-Host "========================================" -ForegroundColor Cyan
Write-Host ""

$scanner = if (Test-Path ".\scanner2.exe") { ".\scanner2.exe" } else { ".\scanner.exe" }

# --- Mandatory 1: Token Classification ---
Write-Host "[Mandatory 1] Token Classification" -ForegroundColor Green
Write-Host "Input: 5 tokens -> int, foo, 3.14, ==, while"
"1","5","int","foo","3.14","==","while","0" | & $scanner
Write-Host ""

# --- Mandatory 2: Code Line Analysis ---
Write-Host "[Mandatory 2] Code Line Analysis" -ForegroundColor Green
Write-Host "Input: int x = 5; if (x > 3) return x;"
"2","int x = 5; if (x > 3) return x;","0" | & $scanner
Write-Host ""

# --- Mandatory 3: Scan .src File ---
Write-Host "[Mandatory 3] Scan Source File (.src)" -ForegroundColor Green
Write-Host "Scanning: test.src"
if (Test-Path ".\test.src") {
    "3","test.src","0" | & $scanner
} else {
    Write-Host "  [!] test.src not found. Copy a .src file here first." -ForegroundColor Yellow
}
Write-Host ""

Write-Host "========================================" -ForegroundColor Cyan
Write-Host "  All mandatory functions demonstrated." -ForegroundColor Cyan
Write-Host "========================================" -ForegroundColor Cyan
