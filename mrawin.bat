@echo off
if exist "%~dp0mra.exe" (
	if not exist "%~dp0log\" mkdir "%~dp0log\"
	if not exist "%~dp0mame\" mkdir "%~dp0mame\"
	if not exist "%~dp0mra\" mkdir "%~dp0mra\"
	if not exist "%~dp0copytoMiST\" mkdir "%~dp0copytoMiST\"
	move *.mra "%~dp0mra\" >nul 2>&1
	move *.zip "%~dp0mame\" >nul 2>&1
	forfiles /p "%~dp0mra" /m *.mra /c "cmd /c "%~dp0mra.exe" -A -z "%~dp0mame" -O "%~dp0copytoMiST" "@path" 2> "%~dp0log\%%~nA.txt""
	forfiles /p "%~dp0log" /m *.txt /c "cmd /c if @fsize EQU 0 del @path"
	cd %~dp0copytoMiST
	setlocal enableDelayedExpansion
	for %%f in (*.rom) do (
		set "filename=%%~f"
		for %%A in (A B C D E F G H I J K L M N O P Q R S T U V W X Y Z) do set "filename=!filename:%%A=%%A!"
		ren "%%f" "!filename!" >nul 2>&1
	)
	endlocal
	start %~dp0copytoMiST\
) else (
	echo ERROR: Program mra.exe not found in folder %~dp0.
	echo.
	pause
)
