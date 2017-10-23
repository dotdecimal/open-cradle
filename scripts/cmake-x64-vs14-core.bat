@echo off
set ASTROID_BUILD_TYPE=debug
"%~dp0cmake-x64.bat" -G"Visual Studio 14 Win64" %*
