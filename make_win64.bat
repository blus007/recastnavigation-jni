@echo off

set BUILD_PATH=build\Win64
md %BUILD_PATH%
md %BUILD_PATH%\RecastDemo\Debug
md %BUILD_PATH%\RecastDemo\Release
pushd %BUILD_PATH%
cmake -G "Visual Studio 17 2022" -A x64 ..\..
popd
cmake --build %BUILD_PATH% --config Release