# Mojo-Insects — Claude Code Guidance

## Project Overview

AI-powered audio plugin built with JUCE 8 + ONNX Runtime.

**Architecture:**
- `PluginProcessor` — audio thread owner; `processBlock` is real-time safe (zero heap allocation)
- `PluginEditor` — JUCE GUI component, fully decoupled from DSP logic
- `InferenceEngine` — wraps `Ort::Session`; runs on a `juce::Thread` background thread

**Audio ↔ Inference bridge:** Results are exchanged via `juce::AbstractFifo` (lock-free FIFO). `processBlock` reads from the FIFO; `InferenceEngine` writes to it. No mutexes on the audio thread.

## Common Commands

### Configure
```bash
cmake -B build -DCMAKE_BUILD_TYPE=Release
```

### Build
```bash
cmake --build build --config Release
```

### Test
```bash
ctest --test-dir build -C Release --output-on-failure
```

### Run a single test by name
```bash
ctest --test-dir build -R TestName --output-on-failure
```

### Validate plugin with pluginval
```bash
pluginval --validate-in-process "build/MojoInsects_artefacts/VST3/MojoInsects.vst3"
```

## Dependencies (managed via CPM.cmake)

| Dependency | Source |
|---|---|
| JUCE 8 | `juce-framework/JUCE` @ v8 |
| ONNX Runtime | `microsoft/onnxruntime` prebuilt (static) |
| Catch2 | `catchorg/Catch2` @ v3 |

## Real-Time Safety Rules

- **Never** allocate heap memory in `processBlock`.
- **Never** acquire a mutex/lock in `processBlock`.
- Use `juce::AbstractFifo` for all audio↔inference communication.
- Model loading and inference live entirely in `InferenceEngine` (background thread).

## CI/CD

GitHub Actions workflow at `.github/workflows/build_and_test.yml` runs on every push:
1. CMake configure + build (Release)
2. Catch2 unit tests via `ctest`
3. `pluginval` plugin validation
4. Codesigning (macOS notarization + Windows SignTool) — requires repo secrets

### Required Secrets
- `APPLE_CERT_BASE64` / `APPLE_CERT_PASSWORD` — Developer ID certificate
- `APPLE_TEAM_ID` / `APPLE_ID` / `APPLE_ID_PASSWORD` — Notarization credentials
- `WINDOWS_CERT_BASE64` / `WINDOWS_CERT_PASSWORD` — Authenticode certificate
