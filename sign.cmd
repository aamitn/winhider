echo.
echo.  Use --help/-h for help
@echo off
setlocal
REM Use --pass "<yourpass>" or -p "<yourpass>" for custom password - USE QUOTES
REM Use --signinstaller or -i to sign installer(s) ONLY (Misc\output\*.exe)
REM Use --signall or -a to sign installer(s) BOTH (Misc\output\*.exe) and (Build\bin\Release\*.exe)
REM For no arguments it signs (Build\bin\Release\*.exe) with default password

REM Change to the directory where the script is located
cd /d "%~dp0"

REM Path to signtool and certificate (relative to script location)
set "SIGNTOOL=Misc\signtool.exe"
set "CERT=Misc\BitmutexCert.pfx"
set "PASSWORD=mysecurepass"  REM Default password

REM Directories
set "TARGET_DIR=Build\bin\Release"
set "INSTALLER_DIR=Misc\output"

REM Flags
set "SIGN_INSTALLER_ONLY=0"
set "SIGN_ALL=0"

REM Parse command line arguments
:parse_args
if "%~1"=="" goto done_parse

if "%~1"=="--help" (
    goto show_help
)
if "%~1"=="-h" (
    goto show_help
)
if "%~1"=="--pass" (
    set "PASSWORD=%~2"
    shift
    shift
    goto parse_args
)
if "%~1"=="-p" (
    set "PASSWORD=%~2"
    shift
    shift
    goto parse_args
)
if "%~1"=="--signinstaller" (
    set "SIGN_INSTALLER_ONLY=1"
    shift
    goto parse_args
)
if "%~1"=="-i" (
    set "SIGN_INSTALLER_ONLY=1"
    shift
    goto parse_args
)
if "%~1"=="--signall" (
    set "SIGN_ALL=1"
    shift
    goto parse_args
)
if "%~1"=="-a" (
    set "SIGN_ALL=1"
    shift
    goto parse_args
)
shift
goto parse_args

:done_parse

REM Timestamp server
set "TIMESTAMP=http://timestamp.comodoca.com/authenticode"

if "%SIGN_ALL%"=="1" (
    echo Signing all files in both %TARGET_DIR% and %INSTALLER_DIR%...

    for %%F in ("%TARGET_DIR%\*.exe" "%TARGET_DIR%\*.dll") do (
        echo Signing %%~nxF
        "%SIGNTOOL%" sign /f "%CERT%" /p "%PASSWORD%" /t "%TIMESTAMP%" /fd sha256 "%%~F"
    )
    for %%F in ("%INSTALLER_DIR%\*.exe") do (
        echo Signing installer %%~nxF
        "%SIGNTOOL%" sign /f "%CERT%" /p "%PASSWORD%" /t "%TIMESTAMP%" /fd sha256 "%%~F"
    )

) else if "%SIGN_INSTALLER_ONLY%"=="1" (
    echo Signing installer .exe files in %INSTALLER_DIR%...
    for %%F in ("%INSTALLER_DIR%\*.exe") do (
        echo Signing %%~nxF
        "%SIGNTOOL%" sign /f "%CERT%" /p "%PASSWORD%" /t "%TIMESTAMP%" /fd sha256 "%%~F"
    )
) else (
    echo Signing all .exe and .dll files in %TARGET_DIR%...
    for %%F in ("%TARGET_DIR%\*.exe" "%TARGET_DIR%\*.dll") do (
        echo Signing %%~nxF
        "%SIGNTOOL%" sign /f "%CERT%" /p "%PASSWORD%" /t "%TIMESTAMP%" /fd sha256 "%%~F"
    )
)

if "%errorlevel%"=="0" (
    echo All files signed successfully.
) else (
    echo ERROR: One or more files could not be signed.
    echo Exit code: %errorlevel%
)

PAUSE
EXIT /B %errorlevel%

:show_help
echo.
echo Usage:
echo   --pass "<yourpass>" or -p "<yourpass>"     Uses custom password (quotes required) and sign Release Files at (Build\bin\Release\*.exe)
echo   --signinstaller or -i                      Sign installer(s) ONLY (Misc\output\*.exe)
echo   --signall or -a                            Sign both installers and release files at (Misc\output\*.exe and Build\bin\Release\*.exe)
echo   --help or -h                               Show this help message and exit
echo.
echo   No arguments:                              Signs release files (Build\bin\Release\*.exe and .dll) with default password
echo.
exit /b 0
