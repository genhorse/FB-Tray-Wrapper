@echo off
setlocal enabledelayedexpansion

rem ------------------------------------------------------------------
rem 1. Project Configuration
rem ------------------------------------------------------------------
set "PROJECT_NAME=FB-Tray-Wrapper"
set "ARCH=x64"
set "BASE_DIR=%~dp0"
set "WEBVIEW2_SDK=%BASE_DIR%deps"
set "INSTALL_PATH=%LOCALAPPDATA%\%PROJECT_NAME%"
set "STARTUP_DIR=%APPDATA%\Microsoft\Windows\Start Menu\Programs\Startup"

echo [INFO] Starting build process for %PROJECT_NAME% (%ARCH%)...

rem ------------------------------------------------------------------
rem 2. Setup Visual Studio Environment (Universal Search)
rem ------------------------------------------------------------------
set "VCVARS_PATH="
set "VSWHERE_PATH=%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe"

if exist "%VSWHERE_PATH%" (
    for /f "usebackq tokens=*" %%i in (`"%VSWHERE_PATH%" -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath`) do (
        set "VS_PATH=%%i"
    )
    if defined VS_PATH (
        set "VCVARS_PATH=!VS_PATH!\VC\Auxiliary\Build\vcvarsall.bat"
    )
)

if not defined VCVARS_PATH (
    echo [INFO] Searching for default VS 2019 paths...
    if exist "C:\Program Files (x86)\Microsoft Visual Studio\2019\BuildTools\VC\Auxiliary\Build\vcvarsall.bat" (
        set "VCVARS_PATH=C:\Program Files (x86)\Microsoft Visual Studio\2019\BuildTools\VC\Auxiliary\Build\vcvarsall.bat"
    ) else if exist "C:\Program Files (x86)\Microsoft Visual Studio\2019\Community\VC\Auxiliary\Build\vcvarsall.bat" (
        set "VCVARS_PATH=C:\Program Files (x86)\Microsoft Visual Studio\2019\Community\VC\Auxiliary\Build\vcvarsall.bat"
    )
)

if not defined VCVARS_PATH (
    echo [ERROR] Visual Studio C++ Tools not found. Please install VS 2019/2022.
    pause
    exit /b 1
)

echo [INFO] Setting up environment using: "!VCVARS_PATH!"
call "!VCVARS_PATH!" %ARCH%

rem ------------------------------------------------------------------
rem 3. Check for WebView2 SDK
rem ------------------------------------------------------------------
if not exist "%WEBVIEW2_SDK%\include\WebView2.h" (
    echo [ERROR] WebView2 SDK not found in: "%WEBVIEW2_SDK%"
    pause
    exit /b 1
)

rem ------------------------------------------------------------------
rem 4. Compilation and Linking
rem ------------------------------------------------------------------
echo [INFO] Compiling resources...
rc.exe /nologo /fo "%BASE_DIR%resource.res" "%BASE_DIR%resource.rc"

echo [INFO] Compiling source code...
cl.exe /nologo /W4 /MD /EHsc /std:c++14 /DUNICODE /D_UNICODE /I"%WEBVIEW2_SDK%\include" /c "%BASE_DIR%main.cpp" /Fo"%BASE_DIR%main.obj"

echo [INFO] Linking...
link.exe /nologo /SUBSYSTEM:WINDOWS /OUT:"%BASE_DIR%%PROJECT_NAME%.exe" "%BASE_DIR%main.obj" "%BASE_DIR%resource.res" "%WEBVIEW2_SDK%\native\%ARCH%\WebView2LoaderStatic.lib" shell32.lib user32.lib ole32.lib advapi32.lib gdi32.lib /ENTRY:wWinMainCRTStartup

if errorlevel 1 (
    echo [ERROR] Build failed.
    pause
    exit /b 1
)

rem ------------------------------------------------------------------
rem 5. Local Installation (AppData)
rem ------------------------------------------------------------------
echo [INFO] Installing to: "%INSTALL_PATH%"
if not exist "%INSTALL_PATH%" mkdir "%INSTALL_PATH%"
copy /Y "%BASE_DIR%%PROJECT_NAME%.exe" "%INSTALL_PATH%\"

rem ------------------------------------------------------------------
rem 6. Create Startup Shortcut via PowerShell
rem ------------------------------------------------------------------
echo [INFO] Creating Startup shortcut...
set "PS_SCRIPT=%TEMP%\CreateShortcut.ps1"
echo $s=(New-Object -ComObject WScript.Shell).CreateShortcut('%STARTUP_DIR%\%PROJECT_NAME%.lnk');$s.TargetPath='%INSTALL_PATH%\%PROJECT_NAME%.exe';$s.Save() > "%PS_SCRIPT%"
powershell -ExecutionPolicy Bypass -File "%PS_SCRIPT%"
del "%PS_SCRIPT%"

echo.
echo =========================================
echo    SUCCESS: INSTALLED AND ADDED TO STARTUP
echo =========================================
echo Executable: %INSTALL_PATH%
echo Shortcut:   %STARTUP_DIR%
echo.
pause
