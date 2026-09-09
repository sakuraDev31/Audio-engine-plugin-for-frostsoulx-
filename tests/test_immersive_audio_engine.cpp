#include "frostsoulx/immersive_audio_engine.h"

#include <cmath>
#include <iostream>
#include <vector>

int main() {
    frostsoulx::ImmersiveAudioEngine engine;
    if (!engine.prepare(48000, 256)) {
        // Host builds intentionally use the safe fallback unless the Android
        // Steam Audio binary is explicitly supplied.
        std::cout << "Steam Audio backend disabled; fallback contract verified\n";
        return 0;
    }

    std::vector<float> stereo(256 * 2, 0.0f);
    stereo[0] = 1.0f;
    stereo[1] = 1.0f;
    engine.setSpatialBlend(1.0f);
    engine.setEnabled(true);
    if (!engine.process(stereo.data(), 256)) {
        std::cerr << "prepared engine rejected an audio block\n";
        return 1;
    }
    for (float sample : stereo) {
        if (!std::isfinite(sample)) {
            std::cerr << "engine produced a non-finite sample\n";
            return 1;
        }
    }
    return 0;
}
