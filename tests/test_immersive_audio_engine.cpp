#include "frostsoulx/immersive_audio_engine.h"

#include <algorithm>
#include <cmath>
#include <iostream>
#include <limits>
#include <vector>

int main() {
    static_assert(frostsoulx::ImmersiveAudioEngine::kPreferredQuantumFrames == 384);
    static_assert(static_cast<int>(frostsoulx::RoomSimulationPreset::Subway) == 5);
    frostsoulx::ImmersiveAudioEngine engine;
    engine.setRoomSize(0.85f);
    engine.setDampening(0.35f);
    engine.setStereoWidth(0.9f);
    const auto controls = engine.spaceDesignControls();
    if (std::fabs(controls.roomSize - 0.85f) > 1.0e-6f
        || std::fabs(controls.dampening - 0.35f) > 1.0e-6f
        || std::fabs(controls.width - 0.9f) > 1.0e-6f) {
        std::cerr << "space design controls did not persist slider values\n";
        return 1;
    }
    engine.setRoomSize(std::numeric_limits<float>::infinity());
    engine.setDampening(-2.0f);
    engine.setStereoWidth(3.0f);
    const auto clampedControls = engine.spaceDesignControls();
    if (clampedControls.roomSize != 0.0f
        || clampedControls.dampening != 0.0f
        || clampedControls.width != 1.0f) {
        std::cerr << "space design controls did not clamp invalid values\n";
        return 1;
    }

    constexpr int kFrames = frostsoulx::ImmersiveAudioEngine::kPreferredQuantumFrames;
    if (!engine.prepare(48000, kFrames)) {
        std::cout << "Steam Audio backend disabled; fallback contract verified\n";
        return 0;
    }

    std::vector<float> stereo(kFrames * 2, 0.0f);
    stereo[0] = 1.0f;
    stereo[1] = 0.25f;
    const auto original = stereo;

    engine.setEnabled(false);
    if (engine.process(stereo.data(), kFrames)) {
        std::cerr << "disabled engine unexpectedly processed audio\n";
        return 1;
    }
    if (stereo != original || engine.lastProcessResult() != frostsoulx::ImmersiveProcessResult::Disabled) {
        std::cerr << "disabled path is not transparent\n";
        return 1;
    }

    engine.setSpatialBlend(1.0f);
    engine.setEnabled(true);
    if (!engine.process(stereo.data(), kFrames)) {
        std::cerr << "prepared engine rejected an audio block\n";
        return 1;
    }
    if (engine.lastProcessResult() != frostsoulx::ImmersiveProcessResult::SteamAudioProcessed) {
        std::cerr << "processed block did not report Steam Audio success\n";
        return 1;
    }
    float peak = 0.0f;
    for (float sample : stereo) {
        if (!std::isfinite(sample)) {
            std::cerr << "engine produced a non-finite sample\n";
            return 1;
        }
        peak = std::max(peak, std::fabs(sample));
    }
    if (peak > 0.961f) {
        std::cerr << "engine limiter exceeded output ceiling\n";
        return 1;
    }
    std::cout << "Steam Audio process status and bypass contract verified\n";
    return 0;
}
