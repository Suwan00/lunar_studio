# Lunar Studio — Example Dataset

This folder ships small, pre-processed outputs of the Lunar Studio pipeline so
that downstream consumers (notably [LNEM](https://github.com/viewlab-group/LNEM-dev))
can be exercised end-to-end without having to first run the ISIS preprocessing
pipeline or download any raw EDR files.

## Layout

```
example/
├── apollo15_example/          # Apollo 15 landing site, NACDTM-referenced, site 1
│   ├── *.echo.site1.pho.campt.png        # Radiometrically-calibrated grayscale (one per orbit)
│   └── *.echo.site1.zdepth.parquet       # Per-pixel camera ray + Z-depth table (one per orbit)
└── apollo17_example/          # Apollo 17 landing site, NACDTM-referenced, site 1
    ├── *.echo.site1.campt.png
    └── *.echo.site1_zdepth.parquet
```

Each `*.parquet` row carries the per-pixel information that volumetric neural
renderers consume directly:

| Column | Meaning |
| --- | --- |
| `Sample`, `Line` | Pixel coordinates in the cropped 512×512 site |
| `Latitude`, `Longitude` | Planetocentric ground coordinates |
| `LookDirectionCameraX/Y/Z` | Body-fixed ray direction from the sensor |
| `SlantDistance` | Distance from sensor to terrain along the ray |
| `ZDepth` | Vertical (Z) depth used as depth supervision |
| `PixelValue` | Radiometrically-calibrated radiance |

The accompanying `*.png` files are grayscale renderings of the matching
`PixelValue` column for visual inspection.

## How the example was produced

These files are exactly what the Lunar Studio pipeline emits at the end of
step 6 (`campt` → `csv2parquet.py`) of the [top-level README](../README.md#processing-your-own-region-of-interest).
The command sequence is:

1. Download the Apollo 15 / 17 LROC NAC EDRs.
2. Run `scripts/process_cubes.sh` against the NACDTM shape model.
3. Crop each site with `scripts/crop_cubes.py`.
4. Run `scripts/run_campt_csv2parquet.sh` across the sites.

<!-- TODO: a fully reproducible, copy-pasteable Quick Example walk-through
     (Apollo 15 NACDTM → LNEM training) will be added here shortly. -->

## Downstream use

Drop either of these folders into the LNEM `dataset/` directory and train
directly against the shipped parquet files:

```bash
# see github.com/viewlab-group/LNEM-dev for the training wrapper
bash train_apollo15.sh
```

## Full dataset

The complete Lunar Studio dataset (Apollo 15, Apollo 17, Lacus, Tycho (KPLO
LUTI), Vallis Schröteri, each in SLDEM-referenced and NACDTM-referenced
variants) is **coming soon**. Hosting details will be linked from the
[top-level README](../README.md#dataset).
