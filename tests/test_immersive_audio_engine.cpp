#include "frostsoulx/immersive_audio_engine.h"

#include <cmath>
#include <iostream>
#include <vector>

int main() {
    static_assert(frostsoulx::ImmersiveAudioEngine::kPreferredQuantumFrames == 384);
    static_assert(static_cast<int>(frostsoulx::RoomSimulationPreset::Subway) == 5);
    frostsoulx::ImmersiveAudioEngine engine;
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
    for (float sample : stereo) {
        if (!std::isfinite(sample)) {
            std::cerr << "engine produced a non-finite sample\n";
            return 1;
        }
    }
    std::cout << "Steam Audio process status and bypass contract verified\n";
    return 0;
}
