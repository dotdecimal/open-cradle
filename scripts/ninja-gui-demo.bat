@echo off
taskkill /im gui_demo.exe /f
call "C:\Program Files (x86)\Microsoft Visual Studio 14.0\VC\vcvarsall.bat" amd64
cd %~dp0
cd ..\build
cd x64-rwdi
ninja gui_demo || goto :error
cd gui_demo
copy /y ..\..\..\src\gui_demo\alia.style alia.style
copy /y ..\..\..\src\gui_demo\ca-bundle.crt ca-bundle.crt
copy /y ..\..\..\src\gui_demo\config.txt config.txt
start gui_demo
goto :done
:error
echo Failed to build with error #%errorlevel%.
pause
:done
