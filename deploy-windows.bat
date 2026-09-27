@echo off
setlocal EnableExtensions EnableDelayedExpansion

if /I "%~1"=="/?" goto :help
if /I "%~1"=="-h" goto :help
if /I "%~1"=="--help" goto :help

set "SCRIPT_DIR=%~dp0"
if "%SCRIPT_DIR:~-1%"=="\" set "SCRIPT_DIR=%SCRIPT_DIR:~0,-1%"

set "BUILD_DIR=%SCRIPT_DIR%\build"
set "DEPLOY_DIR=%SCRIPT_DIR%\deploy"
set "APP_LAUNCHER=gambasse.exe"
set "APP_QT=Gambasse.exe"
set "APP_DEPLOY_PATH=%DEPLOY_DIR%\%APP_LAUNCHER%"
set "APP_LIB_PATH=%DEPLOY_DIR%\lib\%APP_LAUNCHER%"

set "CMAKE_CMD="
set "WINDEPLOYQT_CMD="

for %%I in (cmake.exe) do set "CMAKE_CMD=%%~$PATH:I"
for %%I in (windeployqt.exe) do set "WINDEPLOYQT_CMD=%%~$PATH:I"

if not defined CMAKE_CMD (
  for %%P in (
    "C:\src\software\Qt\Tools\CMake_64\bin\cmake.exe"
    "C:\Qt\Tools\CMake_64\bin\cmake.exe"
  ) do (
    if not defined CMAKE_CMD if exist %%~P set "CMAKE_CMD=%%~P"
  )
)

if not defined WINDEPLOYQT_CMD (
  for %%R in ("C:\src\software\Qt" "C:\Qt") do (
    if not defined WINDEPLOYQT_CMD if exist "%%~R" (
      for /f "delims=" %%F in ('where /R "%%~R" windeployqt.exe 2^>nul') do (
        if not defined WINDEPLOYQT_CMD set "WINDEPLOYQT_CMD=%%F"
      )
    )
  )
)

if not defined CMAKE_CMD (
  echo [ERROR] cmake.exe was not found.
  echo         Add CMake to PATH or install Qt Tools ^(CMake_64^).
  exit /b 1
)

if not defined WINDEPLOYQT_CMD (
  echo [ERROR] windeployqt.exe was not found.
  echo         Add ^<Qt^>\bin to PATH or install a Qt kit with binaries.
  exit /b 1
)

echo Using CMake: %CMAKE_CMD%
echo Using windeployqt: %WINDEPLOYQT_CMD%

echo [1/4] Configuring CMake ^(Release^)...
"%CMAKE_CMD%" -S "%SCRIPT_DIR%" -B "%BUILD_DIR%" -G Ninja -DCMAKE_BUILD_TYPE=Release
if errorlevel 1 exit /b 1

echo [2/4] Building project...
"%CMAKE_CMD%" --build "%BUILD_DIR%"
if errorlevel 1 exit /b 1

if not exist "%BUILD_DIR%\%APP_QT%" goto :missing
if not exist "%BUILD_DIR%\%APP_LAUNCHER%" goto :missing

echo [3/4] Preparing deploy directory...
if exist "%DEPLOY_DIR%" rmdir /s /q "%DEPLOY_DIR%"
mkdir "%DEPLOY_DIR%\lib"
if errorlevel 1 exit /b 1

copy /y "%BUILD_DIR%\%APP_LAUNCHER%" "%APP_DEPLOY_PATH%" >nul
if errorlevel 1 exit /b 1
copy /y "%BUILD_DIR%\%APP_QT%" "%APP_LIB_PATH%" >nul
if errorlevel 1 exit /b 1

echo [4/4] Copying Qt dependencies ^(windeployqt^)...
"%WINDEPLOYQT_CMD%" --release --no-translations --qmldir "%SCRIPT_DIR%\qml" "%APP_LIB_PATH%"
if errorlevel 1 exit /b 1

set "MINGW_BIN="
for %%I in (g++) do (
  set "MINGW_BIN=%%~$PATH:I"
)

if defined MINGW_BIN (
  for %%D in (libgcc_s_seh-1.dll libstdc++-6.dll libwinpthread-1.dll) do (
    if not exist "%DEPLOY_DIR%\lib\%%D" (
      if exist "!MINGW_BIN!\..\%%D" copy /y "!MINGW_BIN!\..\%%D" "%DEPLOY_DIR%\lib\" >nul
      if exist "!MINGW_BIN!\%%D" copy /y "!MINGW_BIN!\%%D" "%DEPLOY_DIR%\lib\" >nul
    )
  )
)

if exist "%SCRIPT_DIR%\config.ini" (
  if not exist "%DEPLOY_DIR%\config.ini" (
    copy /y "%SCRIPT_DIR%\config.ini" "%DEPLOY_DIR%\config.ini" >nul
  )
)

if exist "%SCRIPT_DIR%\database.db" (
  copy /y "%SCRIPT_DIR%\database.db" "%DEPLOY_DIR%\database.db" >nul
)

echo.
echo Deployment created at: %DEPLOY_DIR%
echo Main content:
echo   - %APP_DEPLOY_PATH%
echo   - %APP_LIB_PATH%
echo.
echo Ready to package and deploy.
exit /b 0

:missing
echo [ERROR] Build output not found: %BUILD_DIR%\%APP_QT% or %BUILD_DIR%\%APP_LAUNCHER%
exit /b 1

:help
echo Usage:
echo   %~nx0
echo.
echo Builds Release and creates a self-contained deploy\ directory.
exit /b 0
