cmake_minimum_required(VERSION 3.22)
project(VelvetSpace VERSION 2.0.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>") # static CRT: no VC++ redistributable needed
if(APPLE)
  set(CMAKE_OSX_DEPLOYMENT_TARGET "10.13" CACHE STRING "")
  set(CMAKE_OSX_ARCHITECTURES "arm64;x86_64" CACHE STRING "")
endif()

# Use a local JUCE checkout with -DJUCE_DIR=/path/to/JUCE, otherwise fetch JUCE 8.0.4.
set(JUCE_DIR "" CACHE PATH "Path to a JUCE checkout")
if(JUCE_DIR)
  add_subdirectory(${JUCE_DIR} JUCE EXCLUDE_FROM_ALL)
else()
  include(FetchContent)
  FetchContent_Declare(JUCE GIT_REPOSITORY https://github.com/juce-framework/JUCE.git GIT_TAG 8.0.4 GIT_SHALLOW TRUE)
  FetchContent_MakeAvailable(JUCE)
endif()

set(FORMATS VST3 Standalone)
# moduleinfo.json generation runs the built plugin, which is impossible when cross-compiling.
if(CMAKE_CROSSCOMPILING)
  set(VELVET_VST3_MANIFEST FALSE)
  set(FORMATS VST3)
else()
  set(VELVET_VST3_MANIFEST TRUE)
endif()
if(APPLE)
  list(APPEND FORMATS AU)
endif()

juce_add_plugin(VelvetSpace
  COMPANY_NAME "Velvet Audio"
  BUNDLE_ID "com.velvetaudio.velvetspace"
  PLUGIN_MANUFACTURER_CODE Vlva
  PLUGIN_CODE Vlsp
  FORMATS ${FORMATS}
  PRODUCT_NAME "Velvet Space"
  IS_SYNTH FALSE
  NEEDS_MIDI_INPUT TRUE
  NEEDS_MIDI_OUTPUT FALSE
  IS_MIDI_EFFECT FALSE
  EDITOR_WANTS_KEYBOARD_FOCUS TRUE
  COPY_PLUGIN_AFTER_BUILD FALSE
  VST3_CATEGORIES Fx Reverb
  AU_MAIN_TYPE kAudioUnitType_Effect
  VST3_AUTO_MANIFEST ${VELVET_VST3_MANIFEST})

juce_generate_juce_header(VelvetSpace)

target_sources(VelvetSpace PRIVATE Source/PluginProcessor.cpp Source/PluginEditor.cpp)
target_include_directories(VelvetSpace PRIVATE Source)
target_compile_definitions(VelvetSpace PUBLIC
  JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0 JUCE_VST3_CAN_REPLACE_VST2=0 JUCE_DISPLAY_SPLASH_SCREEN=0)
target_link_libraries(VelvetSpace
  PRIVATE juce::juce_audio_utils juce::juce_dsp
  PUBLIC juce::juce_recommended_config_flags juce::juce_recommended_lto_flags juce::juce_recommended_warning_flags)

option(VELVET_BUILD_TESTS "Build the offline DSP test runner" OFF)
if(VELVET_BUILD_TESTS)
  add_executable(velvet_dsp_tests Tests/DspTests.cpp)
  target_include_directories(velvet_dsp_tests PRIVATE Source)
  target_compile_features(velvet_dsp_tests PRIVATE cxx_std_20)
endif()

