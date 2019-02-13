
md build\vs2017
cd build\vs2017

cmake -DLLVM_ENABLE_PROJECTS="clang" -DLLVM_EXTERNAL_NODECPP_SOURCE_DIR=%cd%\..\..\nodecpp -Thost=x64 -G "Visual Studio 15 2017 Win64" ..\..\3rdparty\llvm

msbuild.exe -property:Configuration=Release tools\clang\tools\nodecpp\src\nodecpp-checker\tool\nodecpp-checker.vcxproj
msbuild.exe -property:Configuration=Release tools\clang\tools\nodecpp\src\nodecpp-safe-library\nodecpp-safe-library.vcxproj
msbuild.exe -property:Configuration=Release utils\FileCheck\FileCheck.vcxproj

cd ..\..

