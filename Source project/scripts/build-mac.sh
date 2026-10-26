#!/bin/bash
# Requires Xcode command line tools and CMake (brew install cmake).
set -e
cd "$(dirname "$0")/.."
cmake -B build -G Xcode -DVELVET_BUILD_TESTS=ON
cmake --build build --config Release --parallel
./build/Release/velvet_dsp_tests
mkdir -p ~/Library/Audio/Plug-Ins/VST3 ~/Library/Audio/Plug-Ins/Components
cp -R "build/VelvetSpace_artefacts/Release/VST3/Velvet Space.vst3" ~/Library/Audio/Plug-Ins/VST3/
cp -R "build/VelvetSpace_artefacts/Release/AU/Velvet Space.component" ~/Library/Audio/Plug-Ins/Components/
echo "Installed VST3 + AU. Rescan plug-ins in Ableton Live."
