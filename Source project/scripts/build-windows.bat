@echo off
rem Requires Visual Studio 2022 (Desktop C++ workload) and CMake on PATH.
cmake -B build -G "Visual Studio 17 2022" -A x64 -DVELVET_BUILD_TESTS=ON || exit /b 1
cmake --build build --config Release --parallel || exit /b 1
build\Release\velvet_dsp_tests.exe
echo.
echo Built: build\VelvetSpace_artefacts\Release\VST3\Velvet Space.vst3
echo Copy it to C:\Program Files\Common Files\VST3\
