@echo off
echo This script helps you create the necessary *.arc and *.rom files
echo required by the Arcade cores for MiST.
echo.
echo Before you continue, make sure that
echo - mra.exe is placed in the same folder as this script.
echo - *.mra and *.zip files are placed either
echo   in the same folder as this script or
echo   in subfolders called /mra/ and /zip/ respectively.
echo.
echo Press any key to continue.
pause >nul 2>&1
cls
if exist "%~dp0mra.exe" (
	if not exist "%~dp0log\" mkdir "%~dp0log\"
	if not exist "%~dp0mra\" mkdir "%~dp0mra\"
	if not exist "%~dp0zip\" mkdir "%~dp0zip\"
	if not exist "%~dp0copytoMiST\" mkdir "%~dp0copytoMiST\"
	move *.mra "%~dp0mra\" >nul 2>&1
	move *.zip "%~dp0zip\" >nul 2>&1
	cd "%~dp0mra\"
	for %%A in (*.mra) do (
		echo %~dp0mra.exe -Avz "%~dp0zip" -O "%~dp0copytoMiST" "%%~dpnxA" > "%~dp0log\%%~nA.txt"
		for /f "delims=" %%i in ('%~dp0mra.exe -Avz "%~dp0zip" -O "%~dp0copytoMiST" "%%~dpnxA"') do (
			echo %%i >> "%~dp0log\%%~nA.txt"
		)
	)
	cd %~dp0copytoMiST
	setlocal enableDelayedExpansion
	for %%f in (*.rom) do (
		set "filename=%%~f"
		for %%A in (A B C D E F G H I J K L M N O P Q R S T U V W X Y Z) do set "filename=!filename:%%A=%%A!"
		ren "%%f" "!filename!" >nul 2>&1
	)
	endlocal
	echo The script has finished executing.
	echo.
	echo The *.arc and *.rom files are stored in the folder
	echo.
	echo "%~dp0copytoMiST\".
	echo.
	echo They need to be copied to the SD card of your MiST.
	echo.
	echo For troubleshooting why *.arc and *.rom files for a game could not be created
	echo you can read the log files which are stored in the folder
	echo.
	echo "%~dp0log\".
	echo.
	echo Lastly, all *.mra files were moved to "%~dp0mra\"
	echo and all *.zip files to "%~dp0zip\".
	echo.
	echo Press any key to exit.
	pause >nul 2>&1
) else (
	echo ERROR: mra.exe not found in folder "%~dp0".
	echo.
	echo Press any key to exit.
	pause >nul 2>&1
)
