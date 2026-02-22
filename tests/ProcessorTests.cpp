#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>

#include <juce_audio_basics/juce_audio_basics.h>

// ── Helpers ──────────────────────────────────────────────────────────────────

static juce::AudioBuffer<float> makeSineBuffer (int numChannels, int numSamples,
                                                 float frequency, float sampleRate)
{
    juce::AudioBuffer<float> buffer (numChannels, numSamples);
    for (int ch = 0; ch < numChannels; ++ch)
    {
        auto* data = buffer.getWritePointer (ch);
        for (int i = 0; i < numSamples; ++i)
            data[i] = std::sin (2.0f * juce::MathConstants<float>::pi
                                 * frequency * (float) i / sampleRate);
    }
    return buffer;
}

// ── Gain tests ────────────────────────────────────────────────────────────────

TEST_CASE ("Gain: applying 0 dB leaves amplitude unchanged", "[dsp][gain]")
{
    const float gainLinear = juce::Decibels::decibelsToGain (0.0f);
    REQUIRE (gainLinear == Catch::Approx (1.0f).epsilon (1e-6));

    auto buffer = makeSineBuffer (2, 512, 440.0f, 44100.0f);
    const float before = buffer.getMagnitude (0, 512);
    buffer.applyGain (gainLinear);
    const float after = buffer.getMagnitude (0, 512);

    REQUIRE (after == Catch::Approx (before).epsilon (1e-5));
}

TEST_CASE ("Gain: applying -6 dB halves the amplitude", "[dsp][gain]")
{
    const float gainLinear = juce::Decibels::decibelsToGain (-6.0206f);
    auto buffer = makeSineBuffer (1, 512, 440.0f, 44100.0f);
    const float before = buffer.getMagnitude (0, 512);
    buffer.applyGain (gainLinear);
    const float after = buffer.getMagnitude (0, 512);

    REQUIRE (after == Catch::Approx (before * gainLinear).epsilon (1e-4));
}

// ── AbstractFifo / lock-free exchange ─────────────────────────────────────────

TEST_CASE ("AbstractFifo: write then read returns same values", "[fifo]")
{
    constexpr int kSize = 256;
    juce::AbstractFifo fifo (kSize);
    std::array<float, kSize> storage {};

    const std::array<float, 8> input  = { 1, 2, 3, 4, 5, 6, 7, 8 };
    std::array<float, 8>       output = {};

    // Write
    {
        const auto scope = fifo.write (8);
        REQUIRE (scope.blockSize1 + scope.blockSize2 == 8);
        std::copy (input.begin(), input.begin() + scope.blockSize1,
                   storage.begin() + scope.startIndex1);
        if (scope.blockSize2 > 0)
            std::copy (input.begin() + scope.blockSize1, input.end(),
                       storage.begin() + scope.startIndex2);
    }

    REQUIRE (fifo.getNumReady() == 8);

    // Read
    {
        const auto scope = fifo.read (8);
        std::copy (storage.begin() + scope.startIndex1,
                   storage.begin() + scope.startIndex1 + scope.blockSize1,
                   output.begin());
        if (scope.blockSize2 > 0)
            std::copy (storage.begin() + scope.startIndex2,
                       storage.begin() + scope.startIndex2 + scope.blockSize2,
                       output.begin() + scope.blockSize1);
    }

    REQUIRE (output == input);
    REQUIRE (fifo.getNumReady() == 0);
}

// ── Denormals ─────────────────────────────────────────────────────────────────

TEST_CASE ("ScopedNoDenormals: compiles and constructs without error", "[rt-safety]")
{
    // Just verify the RAII guard can be instantiated (used in processBlock).
    juce::ScopedNoDenormals noDenormals;
    SUCCEED();
}
