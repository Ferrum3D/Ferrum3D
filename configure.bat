@echo off
call "./scripts/find_vcvars.bat"
if %ERRORLEVEL% NEQ 0 pause
cmake --preset=windows-debug-msvc
if %ERRORLEVEL% NEQ 0 pause
cmake --preset=windows-codegen
if %ERRORLEVEL% NEQ 0 pause
rem clangd looks for compile_commands.json in the project root
copy /Y "cmake-build\windows-codegen\compile_commands.json" "compile_commands.json" >nul
if %ERRORLEVEL% NEQ 0 pause
