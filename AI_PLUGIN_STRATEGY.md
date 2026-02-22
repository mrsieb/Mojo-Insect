# Ziel: Professionelles AI-Audio-Plugin mit JUCE & ONNX

## 1. Tooling & Infrastruktur
- Framework: JUCE 8 (CMake API)
- Build-System: CMake 3.25+ mit Pamplejuce-Template (GitHub Actions Integration)
- AI-Engine: ONNX Runtime (Static Linking bevorzugt, um DLL-Hölle in DAWs zu vermeiden)
- OS: Cross-platform (Windows/macOS ARM & Intel)

## 2. CI/CD Anforderungen (DevOps-Fokus)
- Automatisiertes Codesigning (Apple Notarization / Windows SignTool) via GitHub Actions.
- Unit Tests: Catch2 für DSP-Logik.
- Validation: Automatischer Run von 'pluginval' nach jedem Build.

## 3. DSP-Architektur
- Trennung von GUI und Processor (JUCE Standard).
- Real-time Safety: Keine Speicherallokation im `processBlock`. 
- ONNX Inference muss in einem dedizierten Background-Thread oder hochoptimiert (SIMD) im Audio-Thread laufen.
