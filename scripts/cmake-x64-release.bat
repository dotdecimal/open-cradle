@echo off
set ASTROID_BUILD_TYPE=release
"%~dp0cmake-x64.bat" -G"Ninja" -DCMAKE_BUILD_TYPE=release %*
