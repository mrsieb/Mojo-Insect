cmake_minimum_required(VERSION 3.25)

project(MojoInsects VERSION 0.1.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# ── CPM (package manager) ──────────────────────────────────────────────────────
include(cmake/CPM.cmake)

# ── JUCE 8 ────────────────────────────────────────────────────────────────────
CPMAddPackage(
    NAME JUCE
    GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
    GIT_TAG        8.0.4
)

# ── ONNX Runtime (static prebuilt) ────────────────────────────────────────────
# Guard the download behind an option — the full onnxruntime repo is ~1.5 GB.
# Disable by default so CI builds stay fast; enable when you have a pre-built
# binary or want to build from source.
option(MOJO_ENABLE_ONNX "Download and link ONNX Runtime" OFF)

if(MOJO_ENABLE_ONNX)
    CPMAddPackage(
        NAME onnxruntime
        GITHUB_REPOSITORY microsoft/onnxruntime
        VERSION 1.19.2
        DOWNLOAD_ONLY YES  # Built separately via CMake ExternalProject or pre-built binaries
    )
endif()

# Provide a simple imported target so source files can link against it.
# When MOJO_ENABLE_ONNX is OFF this target is empty and source compiles cleanly
# in passthrough mode (no ONNX calls will be reachable via the #ifdef guard).
add_library(onnxruntime_static INTERFACE)

if(MOJO_ENABLE_ONNX)
    # In CI you would download a pre-built static lib matching your platform and
    # set ONNXRUNTIME_ROOT_DIR to point at it.
    if(DEFINED ENV{ONNXRUNTIME_ROOT_DIR})
        set(ORT_ROOT $ENV{ONNXRUNTIME_ROOT_DIR})
    else()
        set(ORT_ROOT "" CACHE PATH "Path to ONNX Runtime installation (include/ and lib/)")
    endif()

    if(ORT_ROOT)
        target_include_directories(onnxruntime_static INTERFACE "${ORT_ROOT}/include")
        if(WIN32)
            target_link_libraries(onnxruntime_static INTERFACE "${ORT_ROOT}/lib/onnxruntime.lib")
        else()
            target_link_libraries(onnxruntime_static INTERFACE "${ORT_ROOT}/lib/libonnxruntime.a")
        endif()
    endif()
endif()

# ── Catch2 ────────────────────────────────────────────────────────────────────
CPMAddPackage(
    NAME Catch2
    GITHUB_REPOSITORY catchorg/Catch2
    VERSION 3.6.0
)
list(APPEND CMAKE_MODULE_PATH "${Catch2_SOURCE_DIR}/extras")

# ── Plugin target ─────────────────────────────────────────────────────────────
juce_add_plugin(MojoInsects
    COMPANY_NAME            "MojoInsects"
    PLUGIN_MANUFACTURER_CODE Mojo
    PLUGIN_CODE             Mins
    FORMATS                 VST3 AU Standalone
    PRODUCT_NAME            "Mojo Insects"

    IS_SYNTH                FALSE
    NEEDS_MIDI_INPUT        FALSE
    NEEDS_MIDI_OUTPUT       FALSE
    IS_MIDI_EFFECT          FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE

    VST3_CATEGORIES         "Fx"
    AU_MAIN_TYPE            kAudioUnitType_Effect
    COPY_PLUGIN_AFTER_BUILD FALSE
)

juce_generate_juce_header(MojoInsects)

target_sources(MojoInsects
    PRIVATE
        source/PluginProcessor.cpp
        source/PluginEditor.cpp
        source/InferenceEngine.cpp
)

target_compile_definitions(MojoInsects
    PUBLIC
        JUCE_WEB_BROWSER=0
        JUCE_USE_CURL=0
        JUCE_VST3_CAN_REPLACE_VST2=0
)

if(MOJO_ENABLE_ONNX)
    target_compile_definitions(MojoInsects PUBLIC MOJO_ONNX_ENABLED=1)
endif()

target_link_libraries(MojoInsects
    PRIVATE
        juce::juce_audio_utils
        juce::juce_dsp
        onnxruntime_static
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags
)

target_include_directories(MojoInsects PRIVATE source)

# ── Tests ─────────────────────────────────────────────────────────────────────
enable_testing()
add_subdirectory(tests)
