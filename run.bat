@echo off
setlocal enabledelayedexpansion
cd /d "%~dp0"
echo [1/3] Checking GCC with OpenMP support...

set "GCC_BIN="
where gcc >nul 2>nul
if not errorlevel 1 (
  for /f "delims=" %%I in ('where gcc') do (
    if not defined GCC_BIN set "GCC_BIN=%%I"
  )
)

if not defined GCC_BIN (
  for %%D in ("C:\MinGW\bin\gcc.exe" "C:\msys64\mingw64\bin\gcc.exe" "C:\msys64\mingw32\bin\gcc.exe" "C:\mingw64\bin\gcc.exe") do (
    if exist %%D set "GCC_BIN=%%~D"
  )
)

if not defined GCC_BIN (
  echo ERROR: gcc was not found. Install a GCC/MinGW-w64 toolchain with OpenMP support and add its bin folder to PATH.
  pause
  exit /b 1
)

for %%I in ("%GCC_BIN%") do set "GCC_DIR=%%~dpI"
set "PATH=%GCC_DIR%;%PATH%"
echo Using GCC: %GCC_BIN%

echo [2/3] Compiling C/OpenMP backend...
"%GCC_BIN%" -O2 -Wall -Wextra -fopenmp reversal.c -o reversal.exe
if errorlevel 1 (
  echo Compilation failed. Check your GCC installation and OpenMP support.
  pause
  exit /b 1
)

if not exist .venv\Scripts\python.exe (
  echo Creating Python virtual environment...
  py -m venv .venv
  if errorlevel 1 python -m venv .venv
)
if not exist .venv\Scripts\python.exe (
  echo ERROR: Python was not found. Install Python 3 and select Add Python to PATH.
  pause
  exit /b 1
)

echo Installing Flask...
.venv\Scripts\python.exe -m pip install -r requirements.txt
if errorlevel 1 (
  echo Could not install Flask. Check your internet connection.
  pause
  exit /b 1
)

echo [3/3] Starting local web app at http://127.0.0.1:5000
start "" http://127.0.0.1:5000
.venv\Scripts\python.exe app.py
pause
