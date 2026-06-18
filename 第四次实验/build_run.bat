@echo off
chcp 65001 >nul

echo === flex/bison calculator ===
echo.

echo --- test 1: 3 + 5 * 2 ---
echo 3 + 5 * 2 | calc.exe
echo.

echo --- test 2: (4 + 6) * 3 ---
echo (4 + 6) * 3 | calc.exe
echo.

echo --- test 3: 100 / (2 + 3) ---
echo 100 / (2 + 3) | calc.exe
echo.

pause
