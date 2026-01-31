set "VCVARS_PATH="

if exist "C:\Program Files\Microsoft Visual Studio\18\Community\VC\Auxiliary\Build\vcvarsall.bat" (
    set "VCVARS_PATH=C:\Program Files\Microsoft Visual Studio\18\Community\VC\Auxiliary\Build\vcvarsall.bat"
    goto :found
)
if exist "C:\Program Files\Microsoft Visual Studio\18\Professional\VC\Auxiliary\Build\vcvarsall.bat" (
    set "VCVARS_PATH=C:\Program Files\Microsoft Visual Studio\18\Professional\VC\Auxiliary\Build\vcvarsall.bat"
    goto :found
)
if exist "C:\Program Files\Microsoft Visual Studio\18\Enterprise\VC\Auxiliary\Build\vcvarsall.bat" (
    set "VCVARS_PATH=C:\Program Files\Microsoft Visual Studio\18\Enterprise\VC\Auxiliary\Build\vcvarsall.bat"
    goto :found
)

:found
if defined VCVARS_PATH (
    echo Found vcvarsall.bat at: %VCVARS_PATH%
) else (
    echo vcvarsall.bat not found in common locations.
    echo Please ensure Visual Studio or Build Tools are installed.
    pause
    exit 1
)

call "%VCVARS_PATH%" x64
