# FrostSoulX Audio Engine

A portable C++17 DSP core for FrostSoulX’s QQ Music–inspired Sound Field experience and headphone HRTF binaural rendering. The repository intentionally starts with host-independent processors so the same DSP can later be exposed through Android/JNI, Media3, or a native plugin adapter.

## Current scope

The first engine provides a real-time-safe stereo float processor with bypass support, preset defaults, bounded mid/side widening, low-frequency protection, gentle crossfeed, spatial reverb, pseudo-surround enhancement, output gain, and a soft safety limiter. The processor operates on interleaved stereo PCM in place and does not allocate memory from `process()`.

The signal path is deliberately conservative:

> Stereo PCM → low-frequency separation → protected mid/side width and surround enhancement → short asymmetric reflections and diffusion → crossfeed → output gain → soft limiter

The reverb uses fixed-size asymmetric delay buffers to create short early reflections and a compact diffuse tail. The wet path is kept low and is derived mainly from the non-bass signal so the center and low-frequency foundation remain stable. The surround control increases side energy without changing the output channel count; it is therefore pseudo-surround enhancement for stereo playback, not discrete 5.1/7.1 rendering.

This is an app-level immersive enhancement effect. It is not Dolby Atmos decoding, authored multichannel rendering, or head-tracked HRTF spatialization. Those capabilities can be added later as separate adapters or processing layers.

## Presets

| Preset | Intent |
| --- | --- |
| `Natural` | Mild width and crossfeed for everyday listening |
| `Live` | Moderate width and crossfeed with stronger space |
| `Wide` | More instrumental separation while protecting bass |
| `Immersive` | Strongest initial sound-field enhancement with conservative limiting |
| `Custom` | Uses caller-supplied width, crossfeed, protection, surround, and reverb values |

The `intensity` value scales the effect from `0.0` to `1.0`. All public floating-point parameters are sanitized and bounded by the processor. `reverbMix` is limited to a conservative 35% wet maximum, and `reverbDecay` is limited to prevent runaway feedback.

## Build and test

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel
ctest --test-dir build --output-on-failure
./build/frostsoulx_dsp_example
```

## Integration direction

The next adapter should expose the Sound Field processor to Android through JNI or a Media3 `AudioProcessor`. The Media3 adapter must convert supported PCM formats to the engine’s float stereo contract, disable audio offload while the custom DSP is active, flush the processor when stream format changes, and preserve a true bypass path. The platform Android Spatializer should remain a separate path for authored multichannel content so tracks are not double-processed.

## HRTF binaural path

`HrtfBinauralProcessor` is a separate headphone renderer. It accepts measured left and right head-related impulse responses (HRIRs), convolves a mono source with both ear responses, and outputs interleaved stereo headphone audio. The processor supports up to 512 taps, uses fixed storage, and does not allocate during `process()`.

The HRTF processor is deliberately separate from the stereo Sound Field effect. A host should select one spatial path at a time: HRTF binaural for headphone rendering, Sound Field for stereo enhancement, or platform Spatializer for compatible authored multichannel content. The current API accepts HRIR coefficients but does not embed a redistributable HRTF dataset; a SOFA loader and dataset licensing decision are future adapter responsibilities.

Directional HRIR sets can now be supplied with azimuth/elevation coordinates. The renderer selects up to four nearby measurements and performs inverse-distance interpolation over the directional grid. Azimuth wraps across -180/180 degrees, while elevation is clamped to -90/90 degrees. When the direction changes, the old and interpolated new filters are rendered in parallel and mixed with an equal-power crossfade, preventing abrupt filter changes and reducing clicks or comb-filter artifacts during movement.

The renderer uses a hybrid convolution strategy. Responses up to 256 taps remain on the low-latency direct time-domain FIR path. Longer HRIRs use a fixed-size uniformly partitioned FFT path with 64-sample partitions and 128-point radix-2 transforms. The partitioned path adds one partition of algorithmic latency, but its cost scales with the number of partitions rather than multiplying every input sample by every HRIR tap. The fixed buffers are preallocated and the audio callback performs no dynamic allocation.

The partitioned path supports the same directional crossfade model. When a direction changes, current and target partition spectra are rendered and mixed with an equal-power transition. After the transition, the target spectra become active without rebuilding FFT filters inside the callback.

## RIR early reflections

`EarlyReflectionProcessor` is a separate wet-only room stage. It accepts a stereo room impulse response, convolves the mono source into left/right reflection signals, and applies a bounded wet mix. It does not add dry audio and therefore can be summed with the direct HRTF output without duplicating the direct path:

```cpp
hrtf.process(mono, binauralDirect, frames);
reflections.process(mono, roomWet, frames);
for (std::size_t i = 0; i < frames * 2; ++i) {
    output[i] = binauralDirect[i] + roomWet[i];
}
```

The RIR stage is intended for early reflections rather than a complete late-reverberation model. The RIR should contain the desired reflection delays and gains, and its mix should be kept conservative to preserve localization. The shared `RoomGeometry` model now exposes source position, listener position, room dimensions, reference distance, maximum distance, direct-distance exponent, reflection-distance exponent, and reflection balance.

The HRTF processor applies distance-dependent direct attenuation, while the RIR processor applies an independent reflected gain. With a source at distance `d`, reference distance `r`, and exponent `p`, the normalized gain is approximately `(r / d)^p` within the configured maximum distance. This creates a controllable direct-to-reflected balance rather than treating the room response as a fixed-volume effect. Geometry can be updated from a non-real-time control path before processing; the audio callback only reads precomputed scalar gains.

```cpp
RoomGeometry geometry;
geometry.sourcePosition = {4.0f, 1.5f, 1.2f};
geometry.listenerPosition = {0.0f, 0.0f, 1.2f};
geometry.roomDimensions = {8.0f, 6.0f, 3.0f};
geometry.directDistanceExponent = 1.0f;
geometry.reflectionDistanceExponent = 0.5f;
geometry.reflectionBalance = 0.75f;

hrtf.setRoomGeometry(geometry);
reflections.setRoomGeometry(geometry);
```

## SOFA asset workflow

The runtime does not depend on HDF5. Instead, the offline `tools/sofa_to_fhrtf.py` converter reads a licensed SOFA `SimpleFreeFieldHRIR` dataset with `h5py` and writes a compact validated `.fhrtf` asset. The C++ engine loads that asset outside the audio callback:

```bash
python3 tools/sofa_to_fhrtf.py input.sofa assets/profile.fhrtf
```

Then the host can load and use it as follows:

```cpp
processor.prepare({48000.0});
processor.loadFhrtfAsset("assets/profile.fhrtf");
processor.setCrossfadeSamples(256);
processor.setDirection(45.0f, 10.0f);
processor.setEnabled(true);
```

The binary format stores a version, sample rate, directional measurement count, tap count, and left/right HRIR arrays. It is intentionally an internal derived-asset format; redistribution of the original SOFA dataset remains governed by that dataset’s license. The runtime supports up to 2048 taps and 64 directions in its fixed-capacity configuration.

## Design rules

The audio callback must not allocate, lock, perform file I/O, or depend on UI state. Parameter updates should be copied into the processor from a non-real-time control path. The current implementation keeps state for its low-frequency filter, crossfeed, and fixed reverb delay lines, making reset and track transitions explicit. The design follows established artificial-reverb practice using delay lines and allpass/diffusion concepts, while keeping this first implementation small enough for mobile playback.[1] [2] Stereo decorrelation and widening are intentionally bounded to protect mono compatibility.[3]

## Research references

[1]: https://www.dsprelated.com/freebooks/pasp/Artificial_Reverberation.html

[2]: https://ccrma.stanford.edu/~jos/pasp/Schroeder_Allpass_Sections.html

[3]: https://www.dafx.de/paper-archive/2024/papers/DAFx24_paper_92.pdf

## License

License terms have not yet been selected for this new repository. Add the intended license before publishing or integrating the engine into a distributable application.
