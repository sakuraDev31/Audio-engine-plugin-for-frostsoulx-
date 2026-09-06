#include "frostsoulx/dsp/sound_field.h"

#include <cstddef>
#include <iostream>

int main() {
    frostsoulx::dsp::SoundFieldProcessor processor;
    processor.prepare({48000.0, 2});

    frostsoulx::dsp::SoundFieldParameters parameters;
    parameters.enabled = true;
    parameters.preset = frostsoulx::dsp::SoundFieldPreset::Natural;
    parameters.intensity = 0.75f;
    processor.setParameters(parameters);

    float stereoBuffer[] = {0.2f, 0.1f, 0.25f, 0.12f, 0.3f, 0.15f};
    processor.process(stereoBuffer, 3);

    std::cout << "Processed " << 3 << " stereo frames.\n";
    return 0;
}
