#include "frostsoulx/dsp/early_reflections.h"
#include "frostsoulx/dsp/hrtf_binaural.h"
#include "frostsoulx/dsp/sound_field.h"

#include <cassert>
#include <cstdint>
#include <cstdio>
#include <cmath>
#include <cstddef>
#include <fstream>
#include <iostream>
#include <string>

using frostsoulx::dsp::SoundFieldFormat;
using frostsoulx::dsp::SoundFieldParameters;
using frostsoulx::dsp::SoundFieldPreset;
using frostsoulx::dsp::EarlyReflectionProcessor;
using frostsoulx::dsp::RoomGeometry;
using frostsoulx::dsp::HrtfBinauralProcessor;
using frostsoulx::dsp::SoundFieldProcessor;

namespace {

void testBypass() {
    SoundFieldProcessor processor;
    processor.prepare(SoundFieldFormat{});
    SoundFieldParameters parameters;
    parameters.enabled = false;
    processor.setParameters(parameters);

    float samples[] = {0.25f, -0.5f, 0.75f, 0.1f};
    const float original[] = {0.25f, -0.5f, 0.75f, 0.1f};
    processor.process(samples, 2);
    for (std::size_t i = 0; i < 4; ++i) {
        assert(samples[i] == original[i]);
    }
}

void testMonoPreservation() {
    SoundFieldProcessor processor;
    processor.prepare(SoundFieldFormat{});
    SoundFieldParameters parameters;
    parameters.enabled = true;
    parameters.preset = SoundFieldPreset::Immersive;
    parameters.intensity = 1.0f;
    processor.setParameters(parameters);

    float samples[16];
    for (std::size_t i = 0; i < 8; ++i) {
        samples[i * 2] = 0.2f;
        samples[i * 2 + 1] = 0.2f;
    }
    processor.process(samples, 8);
    for (std::size_t i = 0; i < 8; ++i) {
        assert(std::isfinite(samples[i * 2]));
        assert(std::fabs(samples[i * 2] - samples[i * 2 + 1]) < 1.0e-5f);
    }
}

void testWidthIncreasesSideEnergy() {
    SoundFieldProcessor processor;
    processor.prepare(SoundFieldFormat{});
    SoundFieldParameters parameters;
    parameters.enabled = true;
    parameters.preset = SoundFieldPreset::Wide;
    parameters.intensity = 1.0f;
    processor.setParameters(parameters);

    float samples[96];
    for (std::size_t i = 0; i < 48; ++i) {
        samples[i * 2] = 0.35f;
        samples[i * 2 + 1] = -0.15f;
    }
    processor.process(samples, 48);
    const float inputSide = (0.35f - (-0.15f)) * 0.5f;
    const float outputSide = (samples[94] - samples[95]) * 0.5f;
    assert(std::fabs(outputSide) > std::fabs(inputSide));
}

void testParametersAreClamped() {
    SoundFieldProcessor processor;
    SoundFieldParameters parameters;
    parameters.intensity = 99.0f;
    parameters.width = -4.0f;
    parameters.crossfeed = 99.0f;
    parameters.outputGainDb = 99.0f;
    processor.setParameters(parameters);
    const auto& stored = processor.parameters();
    assert(stored.intensity == 1.0f);
    assert(stored.width == 0.0f);
    assert(stored.crossfeed == 0.35f);
    assert(stored.outputGainDb == 6.0f);
}

void testSpatialReverbCreatesTail() {
    SoundFieldProcessor processor;
    processor.prepare(SoundFieldFormat{48000.0, 2});
    SoundFieldParameters parameters;
    parameters.enabled = true;
    parameters.preset = SoundFieldPreset::Custom;
    parameters.intensity = 1.0f;
    parameters.width = 1.0f;
    parameters.crossfeed = 0.0f;
    parameters.surround = 0.0f;
    parameters.reverbMix = 0.30f;
    parameters.reverbRoomSize = 0.5f;
    parameters.reverbDecay = 0.7f;
    processor.setParameters(parameters);

    float impulse[4096] = {};
    impulse[0] = 1.0f;
    impulse[1] = 1.0f;
    processor.process(impulse, 2048);

    bool foundTail = false;
    for (std::size_t i = 2; i < 2048; ++i) {
        if (std::fabs(impulse[i * 2]) > 1.0e-5f || std::fabs(impulse[i * 2 + 1]) > 1.0e-5f) {
            foundTail = true;
            break;
        }
    }
    assert(foundTail);
}

void testSpatialParametersAreClamped() {
    SoundFieldProcessor processor;
    SoundFieldParameters parameters;
    parameters.surround = 99.0f;
    parameters.reverbMix = 99.0f;
    parameters.reverbRoomSize = -1.0f;
    parameters.reverbDecay = 99.0f;
    processor.setParameters(parameters);
    const auto& stored = processor.parameters();
    assert(stored.surround == 1.0f);
    assert(stored.reverbMix == 0.35f);
    assert(stored.reverbRoomSize == 0.0f);
    assert(stored.reverbDecay == 0.85f);
}

void testHrtfDistanceAttenuation() {
    HrtfBinauralProcessor processor;
    processor.prepare({48000.0});
    const float impulse[] = {1.0f};
    assert(processor.setImpulseResponse({impulse, impulse, 1}));
    processor.setEnabled(true);

    RoomGeometry geometry;
    geometry.sourcePosition = {4.0f, 0.0f, 0.0f};
    geometry.listenerPosition = {0.0f, 0.0f, 0.0f};
    geometry.referenceDistance = 1.0f;
    geometry.directDistanceExponent = 1.0f;
    processor.setRoomGeometry(geometry);
    assert(std::fabs(processor.sourceDistance() - 4.0f) < 1.0e-6f);

    const float mono[] = {1.0f};
    float stereo[2] = {};
    processor.process(mono, stereo, 1);
    assert(std::fabs(stereo[0] - 0.25f) < 1.0e-6f);
    assert(std::fabs(stereo[1] - 0.25f) < 1.0e-6f);

    processor.setDistanceAttenuationEnabled(false);
    processor.reset();
    processor.process(mono, stereo, 1);
    assert(std::fabs(stereo[0] - 1.0f) < 1.0e-6f);
    assert(std::fabs(stereo[1] - 1.0f) < 1.0e-6f);
}

void testHrtfBinauralConvolution() {
    HrtfBinauralProcessor processor;
    processor.prepare({48000.0});
    const float leftHrir[] = {0.0f, 0.8f, 0.0f};
    const float rightHrir[] = {1.0f, 0.0f, 0.0f};
    assert(processor.setImpulseResponse({leftHrir, rightHrir, 3}));
    assert(processor.tapCount() == 3);
    processor.setEnabled(true);

    const float mono[] = {1.0f, 0.0f, 0.0f, 0.0f};
    float stereo[8] = {};
    processor.process(mono, stereo, 4);

    assert(std::fabs(stereo[0] - 1.0f) < 1.0e-6f);
    assert(std::fabs(stereo[1] - 0.0f) < 1.0e-6f);
    assert(std::fabs(stereo[2] - 0.0f) < 1.0e-6f);
    assert(std::fabs(stereo[3] - 0.8f) < 1.0e-6f);
}

void testEarlyReflections() {
    EarlyReflectionProcessor processor;
    processor.prepare({48000.0});
    const float leftRir[] = {0.0f, 0.0f, 0.5f};
    const float rightRir[] = {0.0f, 0.0f, 0.25f};
    assert(processor.setImpulseResponse({leftRir, rightRir, 3}));
    processor.setMix(0.8f);
    processor.setEnabled(true);

    const float mono[] = {1.0f, 0.0f, 0.0f, 0.0f};
    float reflections[8] = {};
    processor.process(mono, reflections, 4);
    assert(std::fabs(reflections[0]) < 1.0e-6f);
    assert(std::fabs(reflections[1]) < 1.0e-6f);
    assert(std::fabs(reflections[4] - 0.4f) < 1.0e-6f);
    assert(std::fabs(reflections[5] - 0.2f) < 1.0e-6f);

    processor.setMix(4.0f);
    assert(std::fabs(processor.mix() - 1.0f) < 1.0e-6f);
    RoomGeometry geometry;
    geometry.sourcePosition = {4.0f, 0.0f, 0.0f};
    geometry.listenerPosition = {0.0f, 0.0f, 0.0f};
    geometry.referenceDistance = 1.0f;
    geometry.reflectionDistanceExponent = 1.0f;
    geometry.reflectionBalance = 0.5f;
    processor.setRoomGeometry(geometry);
    assert(std::fabs(processor.sourceDistance() - 4.0f) < 1.0e-6f);
    assert(std::fabs(processor.reflectionGain() - 0.125f) < 1.0e-6f);
    processor.setDistanceBalanceEnabled(false);
    assert(std::fabs(processor.reflectionGain() - 0.125f) < 1.0e-6f);
    processor.setEnabled(false);
    processor.reset();
    processor.process(mono, reflections, 4);
    for (float sample : reflections) {
        assert(std::fabs(sample) < 1.0e-6f);
    }
}

void testPartitionedLongHrirPath() {
    HrtfBinauralProcessor processor;
    processor.prepare({48000.0});
    processor.setCrossfadeSamples(64);

    std::array<float, 320> left{};
    std::array<float, 320> right{};
    left[0] = 0.8f;
    right[0] = 1.0f;
    left[200] = 0.2f;
    right[200] = 0.1f;
    assert(processor.setImpulseResponse({left.data(), right.data(), left.size()}));
    processor.setEnabled(true);

    std::array<float, 256> mono{};
    std::array<float, 512> stereo{};
    mono[0] = 1.0f;
    processor.process(mono.data(), stereo.data(), mono.size());

    bool foundOutput = false;
    for (float sample : stereo) {
        assert(std::isfinite(sample));
        if (std::fabs(sample) > 1.0e-4f) {
            foundOutput = true;
        }
    }
    assert(foundOutput);
    assert(std::fabs(stereo[0]) < 1.0e-6f);

    const float firstPeak = std::max(std::fabs(stereo[128]), std::fabs(stereo[129]));
    assert(firstPeak > 0.1f);
}

void testFhrtfAssetLoader() {
    const std::string path = "frostsoulx_test.fhrtf";
    {
        std::ofstream output(path, std::ios::binary);
        assert(output.good());
        output.write("FHRIR01\\0", 8);
        const std::uint32_t version = 1;
        const float sampleRate = 48000.0f;
        const std::uint32_t directions = 1;
        const std::uint32_t taps = 2;
        output.write(reinterpret_cast<const char*>(&version), sizeof(version));
        output.write(reinterpret_cast<const char*>(&sampleRate), sizeof(sampleRate));
        output.write(reinterpret_cast<const char*>(&directions), sizeof(directions));
        output.write(reinterpret_cast<const char*>(&taps), sizeof(taps));
        const float azimuth = 0.0f;
        const float elevation = 0.0f;
        const float left[] = {0.75f, 0.0f};
        const float right[] = {1.0f, 0.0f};
        output.write(reinterpret_cast<const char*>(&azimuth), sizeof(azimuth));
        output.write(reinterpret_cast<const char*>(&elevation), sizeof(elevation));
        output.write(reinterpret_cast<const char*>(left), sizeof(left));
        output.write(reinterpret_cast<const char*>(right), sizeof(right));
    }

    HrtfBinauralProcessor processor;
    processor.prepare({44100.0});
    assert(processor.loadFhrtfAsset(path.c_str()));
    assert(processor.directionCount() == 1);
    assert(processor.tapCount() == 2);
    processor.setEnabled(true);
    const float mono[] = {1.0f, 0.0f};
    float stereo[4] = {};
    processor.process(mono, stereo, 2);
    assert(std::fabs(stereo[0] - 0.75f) < 1.0e-6f);
    assert(std::fabs(stereo[1] - 1.0f) < 1.0e-6f);
    std::remove(path.c_str());
}

void testHrtfDirectionalInterpolationAndCrossfade() {
    HrtfBinauralProcessor processor;
    processor.prepare({48000.0});
    processor.setCrossfadeSamples(4);

    const float leftFront[] = {1.0f, 0.0f};
    const float rightFront[] = {1.0f, 0.0f};
    const float leftRight[] = {0.0f, 1.0f};
    const float rightRight[] = {1.0f, 0.0f};
    const float leftTop[] = {0.5f, 0.0f};
    const float rightTop[] = {0.5f, 0.0f};
    const HrtfBinauralProcessor::DirectionalImpulseResponse directions[] = {
        {0.0f, 0.0f, {leftFront, rightFront, 2}},
        {90.0f, 0.0f, {leftRight, rightRight, 2}},
        {0.0f, 90.0f, {leftTop, rightTop, 2}},
    };

    assert(processor.setDirectionalResponses(directions, 3));
    assert(processor.directionCount() == 3);
    assert(processor.setDirection(45.0f, 0.0f));
    assert(std::fabs(processor.azimuthDegrees() - 45.0f) < 1.0e-6f);
    assert(std::fabs(processor.elevationDegrees()) < 1.0e-6f);
    processor.setEnabled(true);

    const float mono[] = {1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f};
    float stereo[16] = {};
    processor.process(mono, stereo, 8);
    for (float sample : stereo) {
        assert(std::isfinite(sample));
    }

    assert(processor.setDirection(450.0f, 120.0f));
    assert(std::fabs(processor.azimuthDegrees() - 90.0f) < 1.0e-6f);
    assert(std::fabs(processor.elevationDegrees() - 90.0f) < 1.0e-6f);
    processor.process(mono, stereo, 8);
    for (float sample : stereo) {
        assert(std::isfinite(sample));
    }
}

void testHrtfBypassAndReset() {
    HrtfBinauralProcessor processor;
    processor.prepare({48000.0});
    const float impulse[] = {1.0f};
    assert(processor.setImpulseResponse({impulse, impulse, 1}));

    const float mono[] = {0.3f, -0.2f};
    float stereo[4] = {};
    processor.process(mono, stereo, 2);
    assert(std::fabs(stereo[0] - mono[0]) < 1.0e-6f);
    assert(std::fabs(stereo[1] - mono[0]) < 1.0e-6f);

    processor.setEnabled(true);
    processor.reset();
    processor.process(mono, stereo, 2);
    assert(std::fabs(stereo[0] - mono[0]) < 1.0e-6f);
    assert(std::fabs(stereo[1] - mono[0]) < 1.0e-6f);
}

void testOutputFiniteAndBounded() {
    SoundFieldProcessor processor;
    processor.prepare(SoundFieldFormat{44100.0, 2});
    SoundFieldParameters parameters;
    parameters.enabled = true;
    parameters.preset = SoundFieldPreset::Immersive;
    parameters.intensity = 1.0f;
    parameters.outputGainDb = 6.0f;
    processor.setParameters(parameters);

    float samples[512];
    for (std::size_t i = 0; i < 256; ++i) {
        samples[i * 2] = 4.0f;
        samples[i * 2 + 1] = -4.0f;
    }
    processor.process(samples, 256);
    for (float sample : samples) {
        assert(std::isfinite(sample));
        assert(std::fabs(sample) <= 1.0f);
    }
}

}  // namespace

int main() {
    testBypass();
    testMonoPreservation();
    testWidthIncreasesSideEnergy();
    testParametersAreClamped();
    testSpatialReverbCreatesTail();
    testSpatialParametersAreClamped();
    testHrtfDistanceAttenuation();
    testHrtfBinauralConvolution();
    testEarlyReflections();
    testPartitionedLongHrirPath();
    testFhrtfAssetLoader();
    testHrtfDirectionalInterpolationAndCrossfade();
    testHrtfBypassAndReset();
    testOutputFiniteAndBounded();
    std::cout << "All Sound Field DSP tests passed.\n";
    return 0;
}
