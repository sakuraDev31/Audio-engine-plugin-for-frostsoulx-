# FrostSoulX Stereo Surround Processor

This repository is now a clean, standalone implementation of the first FrostSoulX surround-processing milestone. It contains no Android, JNI, Media3, Oboe, HRTF, EQ, reverb, loudness normalization, or device-tuning code.

## Scope

`StereoSurroundProcessor` accepts interleaved stereo float PCM and returns interleaved stereo float PCM. It preserves the direct stereo field, derives a bounded ambience signal from the mid/side side channel, protects the low-frequency portion of that signal, applies two short fixed decorrelation paths, and folds a conservative virtual-rear contribution back into left and right.

The implementation is intentionally independent of Android and can be tested as a normal C++17 library.

## Guarantees

When disabled, `process()` returns before touching the input buffer or internal state. When enabled with intensity `0`, it is also transparent. All state is allocated in the object and initialized during `prepare()`. `process()` performs no allocation, locking, file I/O, logging, or dynamic resizing. `reset()` clears delay and filter state.

The processor clamps invalid intensity values to `[0, 1]`, treats non-finite input as zero in the active path, keeps delay lengths bounded, and bounds the surround contribution through remaining-headroom shaping rather than block normalization.

## Build and test

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel
ctest --test-dir build --output-on-failure
```

## Processing stages

```text
L/R input
   ↓
Mid/side analysis
   ↓
Low-frequency side protection
   ↓
Short fixed delays
   ↓
Bounded decorrelation
   ↓
Virtual rear ambience
   ↓
Controlled stereo fold-back
   ↓
L'/R' output
```

The final headroom manager and limiter belong outside this module, in the future FrostSoulX effect graph. This processor does not perform full-block peak scanning or block-wide gain rescaling.
