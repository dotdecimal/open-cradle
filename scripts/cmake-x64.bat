@echo off

:: Check that ASTROID_EXTERNAL_LIB_DIR is set.
IF NOT X%ASTROID_EXTERNAL_LIB_DIR%==X GOTO ASTROID_EXTERNAL_LIB_DIR_SET
echo ASTROID_EXTERNAL_LIB_DIR must be set to the directory where Astroid's
echo external libaries dependencies are installed.
exit /b 1
:ASTROID_EXTERNAL_LIB_DIR_SET

:: If using Qt, we need to tell qmake where Qt is located.
echo [Paths] >"%ASTROID_EXTERNAL_LIB_DIR%/qt/bin/qt.conf"
echo Prefix = %ASTROID_EXTERNAL_LIB_DIR%/qt >>"%ASTROID_EXTERNAL_LIB_DIR%/qt/bin/qt.conf"

:: Call cmake.
cmake -fprofile-arcs -ftest-coverage -fPIC -O0 -DCRADLE_SOURCE_PATH=%dp0%\.. -DCMAKE_INCLUDE_PATH=%ASTROID_EXTERNAL_LIB_DIR%/breakpad/include;%ASTROID_EXTERNAL_LIB_DIR%/devil/include;%ASTROID_EXTERNAL_LIB_DIR%/glew/include;%ASTROID_EXTERNAL_LIB_DIR%/libcurl/include;%ASTROID_EXTERNAL_LIB_DIR%/msgpack/include;%ASTROID_EXTERNAL_LIB_DIR%/openssl/include;%ASTROID_EXTERNAL_LIB_DIR%/postgresql/include; -DCMAKE_LIBRARY_PATH=%ASTROID_EXTERNAL_LIB_DIR%/breakpad/lib;%ASTROID_EXTERNAL_LIB_DIR%/devil/lib;%ASTROID_EXTERNAL_LIB_DIR%/glew/lib;%ASTROID_EXTERNAL_LIB_DIR%/libcurl/lib;%ASTROID_EXTERNAL_LIB_DIR%/msgpack/lib;%ASTROID_EXTERNAL_LIB_DIR%/openssl/lib;%ASTROID_EXTERNAL_LIB_DIR%/skia/lib/;%ASTROID_BUILD_TYPE%%ASTROID_EXTERNAL_LIB_DIR%/postgresql/lib; -DZLIB_ROOT=%ASTROID_EXTERNAL_LIB_DIR%/zlib -DSKIA_INCLUDE_DIR=%ASTROID_EXTERNAL_LIB_DIR%/skia/include -DDCMTK_DIR=%ASTROID_EXTERNAL_LIB_DIR%/dcmtk -DBOOST_INCLUDEDIR=%ASTROID_EXTERNAL_LIB_DIR%/boost/include -DBOOST_LIBRARYDIR=%ASTROID_EXTERNAL_LIB_DIR%/boost/lib -DBoost_USE_STATIC_LIBS=ON -DQT_QMAKE_EXECUTABLE=%ASTROID_EXTERNAL_LIB_DIR%/qt/bin/qmake.exe -DwxWidgets_ROOT_DIR=%ASTROID_EXTERNAL_LIB_DIR%/wx -DGLPK_ROOT_DIR=%ASTROID_EXTERNAL_LIB_DIR%/glpk --no-warn-unused-cli %*
