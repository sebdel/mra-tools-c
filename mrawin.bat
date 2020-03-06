@echo off
setlocal enableDelayedExpansion
chcp 28605>nul 2>&1
color 0a
:: give usage instructions
echo Hi %username%^^!
echo.
echo This script helps you create the necessary *.arc and *.rom files
echo required by the Arcade cores for MiST.
echo.
echo Before you continue, make sure that
echo - mra.exe and
echo - the required *.mra and *.zip files
echo are all placed in the same folder as this script.
echo.
choice /m "Continue"
if %errorlevel% equ 2 exit
cls
if not exist "mra.exe" echo ERROR: mra.exe not found in the same folder as the script.&goto END
:: create subfolders if necessary
if not exist ".\copytoMiST\" mkdir ".\copytoMiST\"
if not exist *.mra echo ERROR: Couldn't find any *.mra files.&goto END
if not exist *.zip echo ERROR: Couldn't find any *.zip files.&echo The script will continue, but *.rom files won't be created.&echo.
:: process input files
echo Processing *.mra files ...
set /a errnum=0
for %%A in (*.mra) do (
	echo.
	echo *** %%~nxA ***
	mra.exe -AO ".\copytoMiST" "%%~nxA"
)
:: rename created *.rom files to uppercase and delete empty *.rom files
cd .\copytoMiST
for %%B in (*.rom) do (
	set "filesize=%%~zB"
	set "filename=%%~B"
	if !filesize! gtr 0 (
		for %%C in (A B C D E F G H I J K L M N O P Q R S T U V W X Y Z) do set "filename=!filename:%%C=%%C!"
		ren "%%B" "!filename!">nul 2>&1
	) else (
		del !filename!
	)
)
:: inform user of the outcome
echo.
echo --------------------------------------------------------------------------------
echo.
echo The script has finished executing.
echo.
echo The *.arc and *.rom files are stored in the subfolder "\copytoMiST\".
echo They need to be copied over to the SD card of your MiST.
goto END
:END
echo.
echo Press any key to exit.
pause>nul 2>&1
endlocal
