@echo off

setlocal EnableDelayedExpansion

:: ASTROID PATH RESOLUTION

:: Walk up through the ancestor directories of this batch file until we find a
:: "src" directory. If we find one, we assume its parent is the Astroid root.

for %%a in (%~dp0) do set drive=%%~Da
set batch_path=%~dp0

:: Delete drive from batch_path (from beggining of path until first colon)
set batch_path=%batch_path:*:=%

:: Change possible spaces in the path by another character (I used dollar sign)
:: to avoid separate names with spaces at that point
set batch_path=%batch_path: =$%
:: ... of course, dollars must be returned back to spaces later

:: Extract each partial path from batch_path and search for "src" in it
set dir=
for %%a in (%batch_path:\= %) do (
   set dir=!dir!\%%a
   :: Get back spaces converted into dollars
   set dir=!dir:$= !
   :: Enclose file name in quotes (required if path contain spaces)
   if exist "%drive%!dir!\src" goto found_root_dir
)
echo error: Couldn't find Astroid root directory.
exit /b 1
:found_root_dir
set astroid_root_dir=%drive%%dir%

:: Now resolve the paths of the subdirectories

:: source dir
set astroid_source_dir=%astroid_root_dir%\src

:: build dir
set astroid_build_dir=%astroid_root_dir%\build

:: END OF ASTROID PATH RESOLUTION

set ASTROID_SOURCE_DIR=%astroid_source_dir:\=/%

:: Create the builds.
set build_type=debug
call :create_build
set build_type=rwdi
call :create_build
set build_type=release
call :create_build
set build_type=vs14
call :create_build
:: set build_type=qtcreator
:: call :create_build

goto :EOF

:create_build
    cd \
    set build_dir=%astroid_build_dir%\x64-%build_type%
	if %build_type%==qtcreator goto skip_delete_dir
    echo Removing and recreating %build_dir%.
    rd /s /q "%build_dir%"
:skip_delete_dir
    mkdir "%build_dir%"
    cd "%build_dir%"
    call "%astroid_source_dir%\scripts\cmake-x64-%build_type%.bat" -DCRADLE_INCLUDE_GUI=ON -DCRADLE_INCLUDE_APP_CONFIG=ON -DCRADLE_INCLUDE_WEB_IO=ON -DCRADLE_INCLUDE_STANDARD_IMAGE_IO=ON %astroid_source_dir%
    goto :EOF
