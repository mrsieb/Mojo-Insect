add_executable(MojoInsectsTests
    ProcessorTests.cpp
)

target_link_libraries(MojoInsectsTests
    PRIVATE
        Catch2::Catch2WithMain
        juce::juce_audio_basics
        juce::juce_audio_processors
        juce::juce_recommended_config_flags
        juce::juce_recommended_warning_flags
)

target_compile_definitions(MojoInsectsTests
    PRIVATE
        JUCE_WEB_BROWSER=0
        JUCE_USE_CURL=0
)

# Register tests with CTest via Catch2's CMake integration.
include(CTest)
include(Catch)
catch_discover_tests(MojoInsectsTests)
