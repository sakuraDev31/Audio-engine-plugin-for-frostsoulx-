#include "frostsoulx/StereoSurroundProcessor.h"

#include <algorithm>
#include <cassert>
#include <cmath>
#include <cstddef>
#include <iostream>
#include <limits>
#include <vector>

namespace {

bool nearlyEqual(float a, float b, float epsilon = 1.0e-6f) {
    return std::fabs(a - b) <= epsilon;
}

void prepare(frostsoulx::StereoSurroundProcessor& processor) {
    processor.prepare(48000.0, 2, 1024);
}

void testTrueBypass() {
    frostsoulx::StereoSurroundProcessor processor;
    prepare(processor);
    processor.setEnabled(false);

    std::vector<float> input{0.1f, -0.2f, 0.75f, -0.91f, 0.0f, 0.33f};
    const auto original = input;
    processor.process(input.data(), 3);
    assert(input == original);

    processor.setEnabled(true);
    processor.setIntensity(0.0f);
    processor.process(input.data(), 3);
    assert(input == original);
}

void testCenterStability() {
    frostsoulx::StereoSurroundProcessor processor;
    prepare(processor);
    processor.setEnabled(true);
    processor.setIntensity(1.0f);

    std::vector<float> input(480 * 2, 0.4f);
    const auto original = input;
    processor.process(input.data(), 480);
    for (std::size_t i = 0; i < input.size(); i += 2) {
        assert(nearlyEqual(input[i], input[i + 1], 1.0e-5f));
        assert(std::fabs(input[i] - original[i]) < 1.0e-4f);
    }
}

void testStereoEffectAndIntensity() {
    std::vector<float> source(512 * 2);
    for (std::size_t i = 0; i < source.size(); i += 2) {
        const float phase = static_cast<float>(i / 2) * 0.09f;
        source[i] = 0.7f * std::sin(phase);
        source[i + 1] = 0.25f * std::sin(phase * 0.73f + 0.4f);
    }

    frostsoulx::StereoSurroundProcessor zero;
    prepare(zero);
    zero.setEnabled(true);
    zero.setIntensity(0.0f);
    auto zeroOutput = source;
    zero.process(zeroOutput.data(), 512);
    assert(zeroOutput == source);

    frostsoulx::StereoSurroundProcessor processed;
    prepare(processed);
    processed.setEnabled(true);
    processed.setIntensity(1.0f);
    auto output = source;
    processed.process(output.data(), 512);

    bool changed = false;
    for (std::size_t i = 0; i < output.size(); ++i) {
        assert(std::isfinite(output[i]));
        changed = changed || !nearlyEqual(output[i], source[i], 1.0e-5f);
    }
    assert(changed);
}

void testSpatialStageDoesNotLimit() {
    frostsoulx::StereoSurroundProcessor processor;
    prepare(processor);
    processor.setEnabled(true);
    processor.setIntensity(1.0f);

    // A high-level opposite-side signal leaves enough side energy for the
    // spatial contribution to exceed full scale. The processor must preserve
    // the linear direct-plus-surround result for the later master stage.
    std::vector<float> input{0.99f, -0.99f};
    processor.process(input.data(), 1);
    assert(input[0] > 1.0f || input[1] < -1.0f);
}

void testResetClearsState() {
    frostsoulx::StereoSurroundProcessor processor;
    prepare(processor);
    processor.setEnabled(true);
    processor.setIntensity(1.0f);

    std::vector<float> warmup(256 * 2, 0.0f);
    for (std::size_t i = 0; i < warmup.size(); i += 2) {
        warmup[i] = 0.8f;
        warmup[i + 1] = -0.8f;
    }
    processor.process(warmup.data(), 256);
    processor.reset();

    std::vector<float> silence(8 * 2, 0.0f);
    processor.process(silence.data(), 8);
    for (float sample : silence) assert(nearlyEqual(sample, 0.0f));
}

void testParameterSafety() {
    frostsoulx::StereoSurroundProcessor processor;
    prepare(processor);
    processor.setIntensity(std::numeric_limits<float>::quiet_NaN());
    assert(processor.intensity() == 0.0f);
    processor.setIntensity(4.0f);
    assert(processor.intensity() == 1.0f);
    processor.setIntensity(-2.0f);
    assert(processor.intensity() == 0.0f);
}

}  // namespace

int main() {
    testTrueBypass();
    testCenterStability();
    testStereoEffectAndIntensity();
    testSpatialStageDoesNotLimit();
    testResetClearsState();
    testParameterSafety();
    std::cout << "StereoSurroundProcessor tests passed\n";
    return 0;
}
