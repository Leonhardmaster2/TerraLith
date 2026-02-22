@echo off
call "C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\VC\Auxiliary\Build\vcvarsall.bat" amd64
cd /d C:\Users\leonh\Documents\TerraLith\TerraLith\build
echo === RECONFIGURING CMAKE ===
cmake .. -DCMAKE_PREFIX_PATH=C:/Qt/6.10.2/msvc2022_64 -DCMAKE_TOOLCHAIN_FILE=C:/vcpkg/scripts/buildsystems/vcpkg.cmake
echo === CMAKE EXIT CODE: %errorlevel% ===
if %errorlevel% neq 0 exit /b %errorlevel%
echo === BUILDING WITH CMAKE ===
cmake --build . --config Release --parallel
echo === BUILD EXIT CODE: %errorlevel% ===
