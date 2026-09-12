# FrostSoulX Immersive Audio Engine

This repository is the **canonical source** for FrostSoulX immersive audio processing. The application repository consumes the engine through an Android/Media3 adapter; engine implementation changes should be made here first and then synchronized into the app integration.

## Current engine

The engine exposes one small real-time-safe C++17 class:

```cpp
#include "frostsoulx/immersive_audio_engine.h"

frostsoulx::ImmersiveAudioEngine engine;
engine.prepare(48000, 8192);       // control thread
engine.setSpatialBlend(1.0f);      // control thread
engine.setEnabled(true);           // control thread
engine.process(interleavedStereo, frames); // audio thread
```

The production backend is **Steam Audio 4.8.1**. It creates one Steam Audio context, the default HRTF, and one binaural effect per continuous playback stream. Input is interleaved stereo float PCM at the public engine boundary. Steam Audio receives preallocated deinterleaved channel buffers and returns stereo output, which is interleaved again before returning to the host.

The engine does not allocate, lock, perform file I/O, or log inside `process()`. Allocation and Steam Audio object creation happen in `prepare()`, while stream state is cleared by `reset()` on a control/lifecycle boundary.

## Processing path

```text
Media3 PCM
  → FrostSoulX Media3 adapter
  → interleaved PCM16/float conversion
  → ImmersiveAudioEngine
  → Steam Audio default HRTF binaural effect
  → interleaved PCM output
  → Media3 AudioSink / AudioTrack
```

When the engine is disabled, the app adapter performs a strict renderer-level bypass and does not call the native processing function. This is intentionally separate from `setSpatialBlend(0.0f)`: zero blend is a processed path, while disabled is a byte-preserving fallback path.

## Repository layout

| Path | Purpose |
| --- | --- |
| `include/frostsoulx/immersive_audio_engine.h` | Stable host-facing engine API. |
| `src/immersive_audio_engine.cpp` | Steam Audio-backed implementation and safe fallback. |
| `third_party/steamaudio_sdk/` | Official Steam Audio headers, Android ARM64 library, and Apache-2.0 license. |
| `tests/test_immersive_audio_engine.cpp` | Host smoke test for fallback and finite-output behavior. |
| `CMakeLists.txt` | Host test and Android ARM64 integration build contract. |

## Build and test

Host builds intentionally compile without Steam Audio unless the platform-specific binary is supplied:

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel
ctest --test-dir build --output-on-failure
```

The host test verifies that the fallback contract is safe. The vendored Steam Audio binary in this repository is Android ARM64 only. Android integration enables it with:

```bash
cmake \
  -S . \
  -B build-android-arm64 \
  -DANDROID_ABI=arm64-v8a \
  -DFROSTSOULX_USE_STEAM_AUDIO=ON
```

The FrostSoulX app’s Gradle/CMake layer supplies the Android toolchain and packages `libphonon.so`; this repository does not own the app’s JNI or Media3 lifecycle code.

## Integration contract for the app

The app adapter must configure the engine only for two-channel PCM16 or PCM float input, preserve the original Media3 format, preallocate a dedicated output buffer, and call `reset()` on flush or format changes. It must preserve queue, current media item, playback position, play/pause state, repeat mode, shuffle state, playback parameters, and volume when rebuilding the renderer/sink.

The app must keep the OFF path independent of this engine. If Steam Audio cannot be loaded or the engine cannot prepare, the adapter must fail closed and use the original Media3 audio path rather than creating a partial or silent processor.

The app’s Android AudioEffect layer—equalizer, bass boost, virtualizer, and loudness controls—is a separate platform path and is not implemented in this repository.

## Steam Audio and licensing

The engine uses the official Steam Audio C API package. The SDK headers and Android ARM64 library are vendored under `third_party/steamaudio_sdk/`. The accompanying Apache-2.0 license is included at `third_party/steamaudio_sdk/LICENSE.md`; redistribution must preserve the license and attribution notices.

The default HRTF is supplied by Steam Audio. In addition to binaural rendering, the engine now includes a lightweight post-space stage with preset room simulations (small room, studio, concert hall, cathedral), early reflections, and bounded-feedback reverb tuned for mobile CPU and memory limits.

## Design rules

Changes to the engine should preserve these invariants:

1. `process()` must remain allocation-free, lock-free, non-blocking, and free of file I/O and logging.
2. `setEnabled(false)` must be safe, but the app-level renderer bypass remains the authoritative byte-preserving OFF path.
3. PCM conversion must clamp or reject invalid values and must never emit NaN or infinity.
4. The engine must be reset or recreated when sample rate, channel count, encoding, or maximum frame size changes.
5. Every algorithmic change requires asymmetric stereo test material and measurements at zero and full spatial blend.
6. The application should be updated only after this repository’s native tests and Android ARM64 packaging checks pass.

## Sources

- Steam Audio downloads: https://valvesoftware.github.io/steam-audio/downloads.html
- Steam Audio C API getting started: https://valvesoftware.github.io/steam-audio/doc/capi/getting-started.html
- Steam Audio audio buffers: https://valvesoftware.github.io/steam-audio/doc/capi/audio-buffers.html
- Steam Audio repository: https://github.com/ValveSoftware/steam-audio
- Steam Audio license: https://github.com/ValveSoftware/steam-audio/blob/master/LICENSE.md
