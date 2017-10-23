@echo off
if not "%~1"=="" goto :have-args
echo Usage: clone_gui_test.bat :clone-name
goto :eof
:have-args
cd %~dp0
cd ../sandbox
ROBOCOPY gui_test gui_test_%1 /E /NFL /NDL /NJH /NJS /nc /ns /np

cd ../scripts
python update_clone_paths.py %1
