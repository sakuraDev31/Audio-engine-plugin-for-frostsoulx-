# Spatial Reverb and Surround Enhancement Research

## Findings

Artificial reverberation is commonly built from delay lines, comb filters, and allpass filters. A tapped delay line models discrete reflections, while feedback comb structures and allpass sections create denser reverberant tails. The practical implication is that the first engine should use short, bounded delay networks rather than a large convolution reverb.

Schroeder allpass sections are used as impulse diffusers: they expand a nonzero input sample into a denser impulse response, producing diffusion without changing the magnitude response in the ideal allpass case. A small number of decorrelated allpass stages is therefore appropriate for a lightweight stereo room effect.

Stereo widening and surround-style enhancement must be controlled carefully because excessive inter-channel decorrelation can produce hollow vocals, weak bass, and mono cancellation. The safest approach is to keep the low-frequency and center/mid signal stable, apply widening mostly to the side/high-frequency content, and blend the effect with the dry signal. A surround enhancement mode for stereo content should be presented as pseudo-surround or sound-field enhancement, not as true discrete 5.1/7.1 rendering.

## Proposed implementation

Add a bounded stereo spatial reverb consisting of a short early-reflection tap network and a compact late diffusion network. Keep separate left/right delay lines with asymmetric prime-ish delay lengths to reduce correlation. Feed the reverb from the protected mid/high component, high-pass or attenuate the low end in the wet path, and blend the wet signal at a conservative level.

Add a surround-enhancement control that increases side energy and decorrelation while preserving the mid/center signal and low frequencies. Do not create additional output channels in this first stereo engine. Later, a true multichannel renderer can be added as a separate engine contract.

All state must be preallocated during prepare(). The process() method must not allocate, lock, perform I/O, or resize containers. Parameter changes should be bounded and applied through a control-path setter. Reset must clear delay/reverb state between tracks when requested.

## References

[1]: https://www.dsprelated.com/freebooks/pasp/Artificial_Reverberation.html
[2]: https://ccrma.stanford.edu/~jos/pasp/Schroeder_Allpass_Sections.html
[3]: https://www.dafx.de/paper-archive/2024/papers/DAFx24_paper_92.pdf
[4]: https://aaltodoc.aalto.fi/items/ee8cf4a2-940f-473e-9ca8-c9535ac726d7
