#include "InferenceEngine.h"

#if MOJO_ONNX_ENABLED
  #include <onnxruntime_cxx_api.h>
#endif

//==============================================================================
InferenceEngine::InferenceEngine()
    : juce::Thread ("InferenceEngine")
{
    startThread (juce::Thread::Priority::background);
}

InferenceEngine::~InferenceEngine()
{
    shouldStop.store (true);
    inferenceWakeup.signal();
    stopThread (2000);
}

//==============================================================================
bool InferenceEngine::loadModel (const juce::File& modelFile)
{
    // Must not be called from the audio thread.
    jassert (! juce::MessageManager::getInstance()->isThisTheMessageThread() ||
               juce::MessageManager::existsAndIsCurrentThread());

    if (! modelFile.existsAsFile())
        return false;

#if MOJO_ONNX_ENABLED
    try
    {
        ortEnv     = std::make_unique<Ort::Env> (ORT_LOGGING_LEVEL_WARNING, "MojoInsects");
        Ort::SessionOptions sessionOptions;
        sessionOptions.SetIntraOpNumThreads (1);
        sessionOptions.SetGraphOptimizationLevel (GraphOptimizationLevel::ORT_ENABLE_ALL);

       #if JUCE_WINDOWS
        ortSession = std::make_unique<Ort::Session> (
            *ortEnv, modelFile.getFullPathName().toWideCharPointer(), sessionOptions);
       #else
        ortSession = std::make_unique<Ort::Session> (
            *ortEnv, modelFile.getFullPathName().toRawUTF8(), sessionOptions);
       #endif

        modelLoaded.store (true);
        return true;
    }
    catch (const Ort::Exception& e)
    {
        DBG ("ONNX load failed: " << e.what());
        return false;
    }
#else
    // ONNX not compiled in — mark as ready so the passthrough path works.
    modelLoaded.store (true);
    return true;
#endif
}

//==============================================================================
void InferenceEngine::submitInput (const float* data, int numSamples)
{
    // Audio thread: lock-free write into inputFifo.
    const int toWrite = juce::jmin (numSamples, inputFifo.getFreeSpace());
    if (toWrite == 0)
        return;

    const auto scope = inputFifo.write (toWrite);
    juce::FloatVectorOperations::copy (inputBuffer.data() + scope.startIndex1,
                                       data, scope.blockSize1);
    if (scope.blockSize2 > 0)
        juce::FloatVectorOperations::copy (inputBuffer.data() + scope.startIndex2,
                                           data + scope.blockSize1, scope.blockSize2);

    inferenceWakeup.signal();
}

void InferenceEngine::setOutputFifo (juce::AbstractFifo* fifo, float* buffer) noexcept
{
    outputFifo = fifo;
    outputBuf  = buffer;
}

//==============================================================================
// Inference thread body — never runs on the audio thread.
void InferenceEngine::run()
{
    while (! threadShouldExit())
    {
        inferenceWakeup.wait (100);

        if (threadShouldExit())
            break;

        const int available = inputFifo.getNumReady();
        if (available == 0 || outputFifo == nullptr)
            continue;

        // Read from input FIFO.
        std::vector<float> inputData ((size_t) available);
        {
            const auto scope = inputFifo.read (available);
            juce::FloatVectorOperations::copy (inputData.data(),
                                               inputBuffer.data() + scope.startIndex1,
                                               scope.blockSize1);
            if (scope.blockSize2 > 0)
                juce::FloatVectorOperations::copy (inputData.data() + scope.blockSize1,
                                                   inputBuffer.data() + scope.startIndex2,
                                                   scope.blockSize2);
        }

        // Run inference (or passthrough if not loaded).
        std::vector<float> outputData (inputData.size());

#if MOJO_ONNX_ENABLED
        if (modelLoaded.load() && ortSession)
        {
            try
            {
                Ort::AllocatorWithDefaultOptions allocator;
                auto memInfo = Ort::MemoryInfo::CreateCpu (
                    OrtArenaAllocator, OrtMemTypeDefault);

                const int64_t inputShape[] = { 1, (int64_t) inputData.size() };
                auto inputTensor = Ort::Value::CreateTensor<float> (
                    memInfo, inputData.data(), inputData.size(),
                    inputShape, 2);

                const char* inputNames[]  = { "input"  };
                const char* outputNames[] = { "output" };

                auto outputTensors = ortSession->Run (
                    Ort::RunOptions { nullptr },
                    inputNames,  &inputTensor,  1,
                    outputNames, 1);

                const float* result = outputTensors[0].GetTensorData<float>();
                std::copy (result, result + outputData.size(), outputData.begin());
            }
            catch (const Ort::Exception& e)
            {
                DBG ("Inference error: " << e.what());
                outputData = inputData; // passthrough on error
            }
        }
        else
#endif
        {
            outputData = inputData; // passthrough
        }

        // Write results to output FIFO (read by processBlock).
        const int toWrite = juce::jmin ((int) outputData.size(),
                                        outputFifo->getFreeSpace());
        if (toWrite > 0)
        {
            const auto scope = outputFifo->write (toWrite);
            juce::FloatVectorOperations::copy (outputBuf + scope.startIndex1,
                                               outputData.data(), scope.blockSize1);
            if (scope.blockSize2 > 0)
                juce::FloatVectorOperations::copy (outputBuf + scope.startIndex2,
                                                   outputData.data() + scope.blockSize1,
                                                   scope.blockSize2);
        }
    }
}
