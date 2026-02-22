#pragma once

#include <JuceHeader.h>

// Forward-declare ORT types to avoid pulling onnxruntime_cxx_api.h into every
// translation unit. The .cpp file includes the full header.
namespace Ort { class Session; class Env; }

/**
 * Wraps an ONNX Runtime session and runs inference on a background thread.
 *
 * Thread safety:
 *   - `loadModel()` must be called from the message thread before playback.
 *   - `submitInput()` is called from processBlock (audio thread) and is
 *     lock-free (writes to an AbstractFifo).
 *   - Results are written back to a caller-supplied AbstractFifo.
 */
class InferenceEngine : private juce::Thread
{
public:
    InferenceEngine();
    ~InferenceEngine() override;

    /** Load a .onnx model from disk. Must be called off the audio thread. */
    bool loadModel (const juce::File& modelFile);

    /** Returns true if a model is loaded and ready. */
    bool isReady() const noexcept { return modelLoaded.load(); }

    /** Submit a block of samples for inference (audio thread safe). */
    void submitInput (const float* data, int numSamples);

    /** Attach the output FIFO that processBlock reads from. */
    void setOutputFifo (juce::AbstractFifo* fifo, float* buffer) noexcept;

private:
    void run() override;

    std::atomic<bool> modelLoaded { false };
    std::atomic<bool> shouldStop  { false };

    // Input ring buffer (audio thread → inference thread)
    static constexpr int kInputFifoSize = 4096;
    juce::AbstractFifo inputFifo { kInputFifoSize };
    std::array<float, kInputFifoSize> inputBuffer {};

    // Output FIFO (owned by PluginProcessor, pointer stored here)
    juce::AbstractFifo* outputFifo  { nullptr };
    float*              outputBuf   { nullptr };

    // ORT objects — accessed only on the inference thread after loadModel().
#if MOJO_ONNX_ENABLED
    std::unique_ptr<Ort::Env>     ortEnv;
    std::unique_ptr<Ort::Session> ortSession;
#endif

    juce::WaitableEvent inferenceWakeup;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (InferenceEngine)
};
