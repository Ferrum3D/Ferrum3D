@echo off
call "./scripts/find_vcvars.bat"
if %ERRORLEVEL% NEQ 0 pause
cmake --preset=windows-debug-msvc
if %ERRORLEVEL% NEQ 0 pause
cmake --preset=windows-codegen
if %ERRORLEVEL% NEQ 0 pause
