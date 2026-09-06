#!/usr/bin/env python3
"""Convert a SOFA SimpleFreeFieldHRIR file into FrostSoulX .fhrtf assets.

The runtime stays dependency-free; h5py is required only by this offline tool.
The converter expects Data.IR shaped [M, R, N] with two receivers and SourcePosition
shaped [M, 3] in degrees, using the SOFA SimpleFreeFieldHRIR convention.
"""

from __future__ import annotations

import argparse
import math
import struct
from pathlib import Path


def read_dataset(handle, name):
    if name not in handle:
        raise ValueError(f"missing SOFA dataset: {name}")
    return handle[name][...]


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("input", type=Path, help="input SOFA file")
    parser.add_argument("output", type=Path, help="output .fhrtf file")
    args = parser.parse_args()

    try:
        import h5py
    except ImportError as exc:
        raise SystemExit("Install h5py for the offline converter: python3 -m pip install h5py") from exc

    with h5py.File(args.input, "r") as sofa:
        ir = read_dataset(sofa, "Data.IR")
        positions = read_dataset(sofa, "SourcePosition")
        sampling_rate = float(read_dataset(sofa, "Data.SamplingRate").reshape(-1)[0])

        if ir.ndim != 3 or ir.shape[1] < 2:
            raise ValueError("expected Data.IR with shape [measurements, 2, taps]")
        if positions.ndim != 2 or positions.shape[1] < 2:
            raise ValueError("expected SourcePosition with shape [measurements, 3]")
        if ir.shape[0] != positions.shape[0]:
            raise ValueError("Data.IR and SourcePosition measurement counts differ")
        taps = int(ir.shape[2])
        measurements = int(ir.shape[0])
        if not (1 <= taps <= 2048):
            raise ValueError(f"tap count {taps} is outside the engine limit 1..2048")
        if not (1 <= measurements <= 64):
            raise ValueError(f"measurement count {measurements} is outside the engine limit 1..64")
        if not math.isfinite(sampling_rate) or sampling_rate < 8000 or sampling_rate > 384000:
            raise ValueError("invalid SOFA sampling rate")

        args.output.parent.mkdir(parents=True, exist_ok=True)
        with args.output.open("wb") as output:
            output.write(b"FHRIR01\0")
            output.write(struct.pack("<I f I I", 1, sampling_rate, measurements, taps))
            for index in range(measurements):
                azimuth = float(positions[index, 0])
                elevation = float(positions[index, 1])
                if not math.isfinite(azimuth) or not math.isfinite(elevation):
                    raise ValueError(f"invalid source position at measurement {index}")
                output.write(struct.pack("<f f", azimuth, elevation))
                output.write(struct.pack(f"<{taps}f", *[float(value) for value in ir[index, 0, :]]))
                output.write(struct.pack(f"<{taps}f", *[float(value) for value in ir[index, 1, :]]))

    print(f"wrote {args.output} ({measurements} directions, {taps} taps, {sampling_rate:g} Hz)")


if __name__ == "__main__":
    main()
