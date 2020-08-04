rem mb: Travis-ci uses git-bash to execute scripts under Windows.
rem But vcvars needs to be called under cmd.exe, so this .bat file forces
rem git-bash to launch a cmd.exe to execute this.


cd checker
call checkout.bat
@if ERRORLEVEL 1 exit /b %ERRORLEVEL%


rmdir /S /Q build\travis
mkdir build\travis
cd build\travis

set PATH=C:\ProgramData\chocolatey\lib\sccache\tools\sccache-0.2.12-x86_64-pc-windows-msvc;%PATH%

call "C:\Program Files (x86)\Microsoft Visual Studio\2017\BuildTools\VC\Auxiliary\Build\vcvars64.bat"
@if ERRORLEVEL 1 exit /b %ERRORLEVEL%

cmake -DCMAKE_CXX_COMPILER_LAUNCHER="sccache" -DLLVM_TARGETS_TO_BUILD="X86" -DCMAKE_BUILD_TYPE=Release -DLLVM_ENABLE_PROJECTS="clang" -DLLVM_EXTERNAL_CHECKER_SOURCE_DIR=%cd%\..\.. -G Ninja ..\..\3rdparty\llvm
@if ERRORLEVEL 1 exit /b %ERRORLEVEL%

ninja nodecpp-checker
@if ERRORLEVEL 1 exit /b %ERRORLEVEL%

ninja nodecpp-instrument
@if ERRORLEVEL 1 exit /b %ERRORLEVEL%

ninja nodecpp-safe-library
@if ERRORLEVEL 1 exit /b %ERRORLEVEL%

ninja check-nodecpp-tools
@if ERRORLEVEL 1 exit /b %ERRORLEVEL%

cd ..\..
cd ..
