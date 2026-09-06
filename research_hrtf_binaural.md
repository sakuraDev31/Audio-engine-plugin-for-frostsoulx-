# HRTF Binaural Spatialization Research

## Core findings

SOFA is a standardized file format for spatially oriented acoustic data including HRTFs, HRIRs, BRIRs, and SRIRs. It is standardized by the Audio Engineering Society as AES69 and is intended to support exchange between research and application software. This makes SOFA the right future input format for user-selectable or licensed HRTF datasets.

The 3D Tune-In Toolkit is an open-source C++ reference architecture for real-time binaural spatialization. Its direct path convolves sources with HRIRs, handles interaural time differences separately, and interpolates between measured directions. It separates anechoic/direct rendering from reverberation. It also emphasizes smoothing parameter/filter changes to avoid audible artifacts when positions or listener orientation change.

For this standalone FrostSoulX engine, the first HRTF path should be headphone-only and separate from the existing stereo Sound Field processor. It should not add HRTF processing on top of the pseudo-surround/reverb path by default, because that would double-process spatial cues.

## Recommended first scope

- Start with a single HRTF subject/profile loaded from a built-in or externally supplied dataset.
- Use a compact fixed direction set and nearest-direction selection first; add interpolation after the convolution path is stable.
- Use two HRIRs per direction, one for each ear.
- Use uniform partitioned convolution for real-time processing once HRIR lengths exceed a short direct FIR. A two-stage design can use a short time-domain head and longer FFT partitions for low CPU/latency tradeoff.
- Keep position changes smoothed or crossfaded to prevent clicks and comb-filter artifacts.
- Provide a neutral/bypass path and explicit headphone-only mode.
- Treat dataset licensing separately from code licensing. Do not embed a dataset until its redistribution terms are confirmed.

## References

[1]: https://www.sofaconventions.org/mediawiki/index.php/SOFA_(Spatially_Oriented_Format_for_Acoustics)
[2]: https://journals.plos.org/plosone/article?id=10.1371/journal.pone.0211899
[3]: https://www.sofaconventions.org/mediawiki/index.php/Files
[4]: https://sound.media.mit.edu/resources/KEMAR.html
[5]: https://acoustics.byu.edu/auralization
