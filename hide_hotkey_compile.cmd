@echo off
SETLOCAL ENABLEEXTENSIONS

REM Get the directory where this script is located
SET SCRIPT_DIR=%~dp0

REM Remove trailing backslash if present
IF "%SCRIPT_DIR:~-1%"=="\" SET SCRIPT_DIR=%SCRIPT_DIR:~0,-1%

REM Change directory to the script's directory
cd /d "%SCRIPT_DIR%"

REM Absolute paths
SET AHK_SCRIPT="%SCRIPT_DIR%\hide_hotkey_compile.ahk"
SET AHK_EMBEDDED_EXE="%SCRIPT_DIR%\ahk_embedded\AutoHotkey32.exe"
SET OUTPUT_EXE="hide_hotkey.exe"
SET TARGET_DIR="%SCRIPT_DIR%\Build\bin\Release"

echo Compiling AutoHotkey script: %AHK_SCRIPT%
%AHK_EMBEDDED_EXE% %AHK_SCRIPT%

REM Check if the compilation was successful
IF ERRORLEVEL 1 (
    echo AutoHotkey compilation failed.
    EXIT /B 1
)

echo Checking if target directory exists: %TARGET_DIR%
IF NOT EXIST %TARGET_DIR% (
    echo Target directory does not exist. Creating...
    mkdir %TARGET_DIR%
    IF ERRORLEVEL 1 (
        echo Failed to create target directory.
        EXIT /B 1
    )
)

echo Moving %OUTPUT_EXE% to %TARGET_DIR%
move /Y %OUTPUT_EXE% %TARGET_DIR%
IF ERRORLEVEL 1 (
    echo Failed to move %OUTPUT_EXE%.
    EXIT /B 1
)

echo AutoHotkey script compiled and moved successfully.

ENDLOCAL
echo Exit code: %errorlevel%
PAUSE
EXIT /B 0
