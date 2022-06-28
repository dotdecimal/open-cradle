@echo off
call "C:\Program Files (x86)\Microsoft Visual Studio 14.0\VC\vcvarsall.bat" amd64
cd %~dp0
cd ..\build
cd x64-release
cmake .
cd ..
cd x64-debug
cmake .
cd ..
cd x64-rwdi
cmake .
cd ..
cd x64-vs14
cmake .
pause
