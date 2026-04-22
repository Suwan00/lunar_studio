<div align="center">
<h1>🌕 Lunar Studio</h1>

<p><strong>A standardized preprocessing pipeline for lunar neural rendering and high-fidelity 3D reconstruction</strong></p>

[**Suwan Lee**](https://github.com/viewlab-group)<sup>1</sup> · [**Jo Ryeong Yim**](https://github.com/viewlab-group)<sup>2</sup> · [**Kibaek Park**](https://github.com/viewlab-group)<sup>1</sup> · [**Dong-Gyu Kim**](https://github.com/viewlab-group)<sup>2</sup> · [**Eunhyeuk Kim**](https://github.com/viewlab-group)<sup>2</sup> · [**Minsup Jeong**](https://github.com/viewlab-group)<sup>3</sup> · [**Chae Kyung Sim**](https://github.com/viewlab-group)<sup>3</sup> · [**Seokju Lee**](https://sites.google.com/view/seokju-lee)<sup>1†</sup>

<sup>1</sup>KENTECH &nbsp;&nbsp; <sup>2</sup>KARI &nbsp;&nbsp; <sup>3</sup>KASI

<a href="https://github.com/Suwan00/lunar_studio"><img src="https://img.shields.io/badge/GitHub-Lunar_Studio-blue" alt="GitHub"></a>
<a href="https://hub.docker.com/r/viewlab/lunar-studio"><img src="https://img.shields.io/badge/Docker-viewlab%2Flunar--studio-2496ED?logo=docker&logoColor=white" alt="Docker Hub"></a>
<a href="https://github.com/viewlab-group/LNEM-dev"><img src="https://img.shields.io/badge/Paper-LNEM_(CVPR_2026)-red" alt="LNEM Paper"></a>
<a href="LICENSE"><img src="https://img.shields.io/badge/License-MIT-green" alt="License"></a>

</div>

---

## Overview

**Lunar Studio** is an end-to-end preprocessing toolkit that takes raw lunar orbiter imagery (LROC NAC EDRs, KPLO LUTI observations, and beyond) and produces clean, photogrammetrically-aligned inputs ready for neural rendering pipelines such as [LNEM](https://github.com/viewlab-group/LNEM-dev) (Lunar Neural Elevation Model, CVPR 2026).

Lunar Studio was built to lower the barrier between *planetary data archives* and *modern 3D reconstruction research*. It is **not limited to the datasets we release** — while we ship curated Apollo 15, Apollo 17, Lacus, Tycho (LUTI), and Vallis Schröteri examples, the same pipeline is designed to work on any region of interest on the lunar surface, including **mid-latitude terrains and the south polar region**, wherever LROC NAC (or KPLO LUTI) coverage and a reference DEM are available.

At its core, Lunar Studio is a **thin wrapper around a customised ISIS3 build**, distributed as a single Docker image:

```
viewlab/lunar-studio:1.0.0
```

It combines:

- Our **customised fork of USGS ISIS3** — adds a KPLO LUTI camera model and extends `campt` / `CameraPointInfo` to emit per-pixel body-fixed ray geometry and Z-depth information that volumetric neural renderers can consume directly.
- **[Ames Stereo Pipeline](https://stereopipeline.readthedocs.io/)** 3.5.0 — bundled for downstream DEM/stereo workflows.
- A curated set of **Python + Bash driver scripts** (`process_cubes.sh`, `crop_cubes.py`, `csv2parquet.py`) that wire the ISIS apps into a single reproducible pipeline.

---

## What's inside our ISIS fork

We maintain a public fork of [DOI-USGS/ISIS3](https://github.com/DOI-USGS/ISIS3) with the following additions on top of ISIS 8.1.0:

1. **KPLO LUTI camera model** (`isis/src/luti/objs/LutiCamera/`)
   Adds support for Korea Pathfinder Lunar Orbiter (KPLO) Lunar Terrain Imager observations — camera + distortion model, `Camera.plugin` registration, and the four associated translation files (`KploLutiArchive.trn`, `KploLutiBandBin.trn`, `KploLutiInstrument.trn`, `KploLutiSerialNumber.trn`).

2. **Extended `campt`** (`isis/src/base/apps/campt/campt.cpp`, `campt.xml`)
   Adds rectangular line/sample range output so a single invocation can emit per-pixel geometry for the full image (or an arbitrary window), `format=flat` CSV output, and `allowerror=true` handling for off-limb samples. Adds `useoffset` for sub-pixel-accurate look directions. These flags are what the `csv2parquet.py` step downstream consumes.

3. **Extended `CameraPointInfo`** (`isis/src/base/objs/CameraPointInfo/`)
   The back-end that `campt` calls. Extended to emit **LookDirectionCameraX/Y/Z** (body-fixed ray direction) and **SlantDistance / ZDepth** values per sample/line, which are the primary supervision signals for volumetric rendering.

Everything is built from source inside this repository's [`docker/Dockerfile`](docker/Dockerfile) — no pre-packaged binaries are copied in. The ISIS3 source tree (with our additions) lives alongside it in [`isis/`](isis/), and the build is driven by [`docker/build_image.sh`](docker/build_image.sh). The upstream ISIS3 `README.md` is preserved as [`README_ISIS3.md`](README_ISIS3.md) for reference.

---

## Bundled tools

The Docker image `viewlab/lunar-studio:1.0.0` contains, pre-installed and on `$PATH` after activation:

| Tool | Version | Notes |
| --- | --- | --- |
| ISIS3 (our fork) | 8.1.0 + Lunar Studio patches | Full app suite: `lronac2isis`, `spiceinit`, `lronaccal`, `lronacecho`, `footprintinit`, `seedgrid`, `cnetadd`, `cnetedit`, `pointreg`, `findfeatures`, `jigsaw`, `crop`, `campt`, `camrange`, `isis2std`, … |
| [Ames Stereo Pipeline](https://stereopipeline.readthedocs.io/) | 3.5.0 (2025-04-28 build) | Stereo DEM generation, orthorectification. Available under `/opt/StereoPipeline-3.5.0-2025-04-28-x86_64-Linux/bin`, auto-added to `$PATH`. Upstream: [NeoGeographyToolkit/StereoPipeline](https://github.com/NeoGeographyToolkit/StereoPipeline). |
| Conda environment `lunar_studio` | Python 3.11 | NumPy, Pandas, SciPy, Open3D, matplotlib, plio, spiceypy, h5py, … — everything `csv2parquet.py` and `crop_cubes.py` need. |

The environment is auto-activated on container start through `/usr/local/bin/entrypoint.sh`.

---

## Installation

### Option A — pull the pre-built image (recommended)

```bash
docker pull viewlab/lunar-studio:1.0.0
```

That's it. No conda environment, no ISIS source build, no dependency hell. Image size is ~13 GB (ISIS3, Qt5, PCL, VTK, Open3D, Ames Stereo Pipeline all baked in).

### Option B — build from source

If you want to rebuild from scratch (e.g. to reproduce the binaries or modify the ISIS fork):

```bash
git clone https://github.com/Suwan00/lunar_studio.git
cd lunar_studio/docker
./build_image.sh   # builds image from the ISIS3 sources in this repo
```

Build time is roughly **1.5–3 hours** on a modern workstation — ISIS3 C++ compilation with `ninja -j$(nproc)` dominates. The Dockerfile is fully self-contained: it installs Miniconda, creates the `lunar_studio` conda environment from `docker/environment.yml`, clones our ISIS3 fork, and runs `cmake -GNinja ... && ninja install`.

---

## Prerequisites: the ISIS Data Area

ISIS3 tools require access to a large collection of mission-specific ancillary data (SPICE kernels, sensor models, calibration tables). **This data is not bundled into the Docker image** — it is roughly 1 TB for the full LRO set and grows over time — so you must download it to your host machine once and mount it into the container on every run.

**1. Pick a location on your host with ~1 TB free**, e.g. `/data/isis_data`.

**2. Inside a running container, run the downloader** (see the [USGS ISIS Data Area guide](https://astrogeology.usgs.gov/docs/how-to-guides/environment-setup-and-maintenance/isis-data-area/) for the full list of mission data areas):

```bash
docker run -it --rm \
  -v /data/isis_data:/isis_data \
  -e ISISDATA=/isis_data \
  viewlab/lunar-studio:1.0.0 \
  bash -lc 'downloadIsisData lro $ISISDATA'
```

The LRO data area (needed for any LROC NAC processing) is the largest single download. If you also work with KPLO LUTI imagery, add `downloadIsisData kplo $ISISDATA` (small).

**3. From then on, any container invocation mounts the data area and exports `ISISDATA`:**

```bash
docker run -it \
  -v /data/isis_data:/isis_data \
  -v $(pwd):/workspace \
  -e ISISDATA=/isis_data \
  viewlab/lunar-studio:1.0.0
```

The container's entrypoint captures `ISISDATA` and wires it through ISIS3's standard environment so `campt`, `spiceinit`, and friends find their kernels automatically.

> **Note on `ISISROOT`**: unlike `ISISDATA`, the ISIS install prefix (`ISISROOT=/opt/ISIS3/build`) lives *inside* the container and is set by the conda activation hook — you never touch it on the host.

---

## Quick example

We ship a small example dataset under [`example/`](example/) so you can verify your install without any EDR download or pipeline run. Each region ships the output that the pipeline *produces* (grayscale image PNG + a parquet file with per-pixel camera poses, body-fixed ray directions, and Z-depth) so it can be fed directly into LNEM:

```
example/
├── apollo15_example/
│   ├── M1144779525RE.echo.site1.pho.campt.png
│   ├── M1144779525RE.echo.site1.zdepth.parquet
│   ├── M1356476581RE.echo.site1.pho.campt.png
│   ├── M1356476581RE.echo.site1.zdepth.parquet
│   ├── M183504057LE.echo.site1.pho.campt.png
│   └── M183504057LE.echo.site1.zdepth.parquet
└── apollo17_example/
    ├── M104311715RE.echo.site1.campt.png
    ├── M104311715RE.echo.site1_zdepth.parquet
    ├── M104318871RE.echo.site1.campt.png
    ├── M104318871RE.echo.site1_zdepth.parquet
    ├── M1218752280RE.echo.site1.campt.png
    └── M1218752280RE.echo.site1_zdepth.parquet
```

See [`example/README.md`](example/README.md) for the commands that generated these products and for an end-to-end LNEM training recipe on the Apollo 15 NACDTM variant.

---

## Processing your own region of interest

Below is the full recipe to go from "an interesting region on the Moon" to "a parquet file LNEM can ingest". Every step listed here is exercised by the driver scripts under [`scripts/`](scripts/); you can use them as-is for quick iteration, or copy the relevant ISIS commands into your own pipeline.

### Step 1 — pick a region and download the EDRs

Open [Quick Map](https://quickmap.lroc.im-ldi.com) (the LROC team's interactive tool maintained by Arizona State University). Navigate to your region of interest — the tool supports the entire Moon, including **mid-latitude terrains** and the **south polar region** at [ProjectionSouth with `proj=10`](https://quickmap.lroc.im-ldi.com/?prjExtent=-3758050.0574053%2C-1737400%2C3758050.0574053%2C1737400&stack=3314&proj=10).

Select **3 to 5 LROC NAC EDR observations** that:

- **overlap** sufficiently over your region, and
- exhibit **diverse photometric variation** (different sun azimuth / incidence angles). Shadows that move between acquisitions give neural renderers the supervision signal they need to disentangle albedo from illumination.

Download the `.IMG` files into your working directory.

### Step 2 — grab reference DEMs

For bundle adjustment and depth supervision, download one or both of:

- **NACDTM** — high-resolution (2–5 m/px) local DEMs produced from NAC stereo pairs. Browse at [data.lroc.im-ldi.com](https://data.lroc.im-ldi.com/lroc/rdr_product_select?filter[product_type]=NACDTM); the Apollo 15 variant is available at [NAC_DTM_APOLLO15_2](https://data.lroc.im-ldi.com/lroc/view_rdr/NAC_DTM_APOLLO15_2).
- **SLDEM** — 59 m/px global DEM from LOLA + SELENE, available from [NASA PGDA](https://pgda.gsfc.nasa.gov/products/54).

Convert each DEM into an ISIS shape-model cube using `demprep` (or `dem2isis`) before use. See the [LROC NAC Processing Guide](https://lroc.im-ldi.com/data/support/downloads/LROC_NAC_Processing_Guide.pdf) produced by the ASU LROC team for the canonical recipe.

### Step 3 — launch the container

```bash
docker run -it \
  -v /data/isis_data:/isis_data \
  -v $(pwd):/workspace \
  -e ISISDATA=/isis_data \
  viewlab/lunar-studio:1.0.0
```

All subsequent ISIS commands run *inside* this shell. Your working directory with the EDRs and DEM cubes is mounted at `/workspace`.

### Step 4 — run the preprocessing pipeline

The driver script [`scripts/process_cubes.sh`](scripts/process_cubes.sh) walks the following stages. You can invoke it end-to-end or read the sections below to understand what each stage does and how to override its parameters.

| Stage | ISIS app | What it does | Key tunable |
| --- | --- | --- | --- |
| 1 | `lronac2isis from=*.IMG to=*.raw.cub` | Convert PDS `.IMG` to native ISIS cube. | — |
| 2 | `spiceinit shape=user model=<DEM>.cub CKSMITHED=True SPKSMITHED=True` | Attach SPICE ephemeris + point-to-terrain via your DEM. | `model=` → NACDTM or SLDEM |
| 3 | `lronaccal` | Radiometric calibration (DN → I/F). | — |
| 4 | `lronacecho` | Remove LROC electronic "echo" artefact. | — |
| 5 | `footprintinit linc=30 sinc=30 increaseprecision=true` | Compute ground footprint polygons. | `linc`/`sinc` for density |
| 6 | `seedgrid target=moon minlat=… maxlat=… minlon=… maxlon=… latstep=0.05 lonstep=0.05` → `cnetadd` → `cnetedit` | Build a regular control-network of ground points over your ROI. | `minlat/maxlat/minlon/maxlon`, `latstep`, `lonstep` |
| 7 | `pointreg` with maximum-correlation chips | Sub-pixel refinement of control points. | PatternChip / SearchChip size in the `.def` file |
| 8 | `jigsaw update=yes camsolve=velocities spsolve=positions heldlist=held.lis outlier_rejection=yes` | Bundle adjustment — solves per-image pose corrections. | `maxits`, `sigma`, `rejection_multiplier` |

`process_cubes.sh` exposes every parameter as a variable at the top of the file — override by `env VAR=value bash process_cubes.sh`, by editing the file directly, or by copying it as a template for a new region.

> **When to use `seedgrid` vs `findfeatures`.** `seedgrid` builds a regular lat/lon grid of seed points and is stable for full-cube (large) images where feature descriptors are unreliable at the edges. `findfeatures` (SIFT) is what we use *after* cropping to small 512×512 sites — see Step 5. `process_cubes.sh` picks the right one based on whether you're in full-cube or site mode.

### Step 5 — crop sites of interest

For training, we cut small 512×512 "sites" centred on the pixel you care about. [`scripts/crop_cubes.py`](scripts/crop_cubes.py) does this, using a **reference cube** + a list of **(sample, line)** centre pixels; it then uses `campt`/`camrange` to find the matching crop window on every *other* cube (so that all site-cropped images cover the same physical ground patch even if they were acquired from different orbits).

```bash
python3 scripts/crop_cubes.py \
    --working-dir /workspace/<region> \
    --reference M1114512054LE \
    --others M1114497847LE M1114504950LE \
    --sites-csv sites.csv \
    --crop-size 512
```

where `sites.csv` is a simple 3-column file (header row required):

```csv
site_id,sample,line
1,2532,40047
2,2258,34102
3,2645,28492
4,2600,27333
5,2552,19674
```

Each row becomes a separate `site<id>/` sub-directory under `--working-dir`, containing the reference cube cropped at `(sample, line)` centre and every image in `--others` cropped to the same physical ground patch. Want a different ROI? Edit `sites.csv` — no code changes required. Want to run a single ad-hoc site without writing a CSV? Pass `--site-id 1 --sample 2532 --line 40047` directly. After cropping, `process_cubes.sh --mode site` re-runs the control-network / `findfeatures` / `jigsaw` loop at the site level to lock sub-pixel pose in the tight crop.

### Step 6 — extract rays and convert to parquet

Once the cubes are bundle-adjusted, extract per-pixel camera geometry with our extended `campt`:

```bash
campt from=<image>.echo.site1.cub \
      to=<image>.echo.site1.csv \
      useoffset=true \
      line=1 nlines=512 sample=1 nsamples=512 \
      format=flat \
      allowerror=true
```

This emits one row per pixel with columns including `Sample`, `Line`, `Latitude`, `Longitude`, `LookDirectionCameraX/Y/Z`, `SlantDistance`, and pixel radiance. Then convert the CSV to a compact parquet plus image/pointcloud bundle with:

```bash
python3 scripts/csv2parquet.py --directory /workspace/<region>/<site>
```

The resulting `*.zdepth.parquet` + `*.pho.campt.png` pair is exactly the format LNEM ingests.

The orchestration script [`scripts/run_campt_csv2parquet.sh`](scripts/run_campt_csv2parquet.sh) runs both of the above in parallel across all sites of a region — edit its `SITES=(…)` array to match your layout.

### Step 7 — train LNEM

Point LNEM at your new parquet directory and go. See the [LNEM repository](https://github.com/viewlab-group/LNEM-dev) for training scripts, hyperparameters, and evaluation tooling.

---

## Scripts

All driver scripts live under [`scripts/`](scripts/) and are written to be readable and parameterisable. Highlights:

- **`process_cubes.sh`** — the full ISIS pipeline (Step 4 + the site-mode variant of Step 5's post-crop bundle adjustment). Top of file exposes `MIN_LAT`, `MAX_LAT`, `MIN_LON`, `MAX_LON`, `LATSTEP`, `LONSTEP`, `SHAPE_MODEL`, `MAX_JIGSAW_ITERATIONS`, `SIGMA`, `REFERENCE_ID`, `NUM_SITES`, `ENABLE_OUTLIER_REJECTION`.
- **`crop_cubes.py`** — site cropping (Step 5). Reference image and site centre coordinates are `--args`; no hardcoded region names.
- **`csv2parquet.py`** — CSV → parquet + PNG + PLY pointcloud pipeline (Step 6). Accepts `--input`, `--directory`, or `--regions`.
- **`run_campt_csv2parquet.sh`** — parallel driver that launches Step 6 across many sites with configurable `MAX_PARALLEL` concurrency.

---

## Dataset

Release plan:

- **Example data** — [`example/`](example/) ships mini Apollo 15 / Apollo 17 parquet + PNG bundles so anyone can run LNEM without going through the full pipeline. ✅ Available at initial release.
- **Full Lunar Studio dataset** — Apollo 15, Apollo 17, Lacus, Tycho (KPLO LUTI), Vallis Schröteri in both SLDEM-referenced and NACDTM-referenced variants. 🚧 **Coming soon** — will be linked here once hosting is finalised.

For custom regions, follow the [Processing your own region of interest](#processing-your-own-region-of-interest) workflow above.

---

## Acknowledgements

Lunar Studio would not exist without the work of several open-source communities and research groups:

- **USGS Astrogeology Science Center** for [ISIS3](https://github.com/DOI-USGS/ISIS3) — every command in the Lunar Studio pipeline is an ISIS app (or a short wrapper around one). Our fork sits on top of ISIS 8.1.0 and upstreams should be consulted for anything touching the core apps.
- **Arizona State University — LROC Team** for the [Quick Map](https://quickmap.lroc.im-ldi.com) interactive browser and the [LROC NAC Processing Guide](https://lroc.im-ldi.com/data/support/downloads/LROC_NAC_Processing_Guide.pdf) that informed our DEM preparation and bundle-adjustment recipes.
- **NASA Ames** and the **NeoGeographyToolkit** team for the [Ames Stereo Pipeline](https://stereopipeline.readthedocs.io/), bundled inside every Lunar Studio image.
- **Korea Aerospace Research Institute (KARI)** for access to KPLO LUTI data and instrument documentation, which enabled the LUTI camera model in our ISIS fork.

This research was supported by the Korea Astronomy and Space Science Institute under the R&D program (Project No. 2025-1-850-07) supervised by the Ministry of Science and ICT, and by the Basic Science Research Program through the National Research Foundation of Korea (NRF) funded by the Ministry of Education (No. RS-2024-00463470).

---

## Citation

If Lunar Studio is useful in your research, please cite LNEM:

```bibtex
@inproceedings{lee2026lnem,
  author    = {Suwan Lee and Jo Ryeong Yim and Kibaek Park and Dong-Gyu Kim and
               Eunhyeuk Kim and Minsup Jeong and Chae Kyung Sim and Seokju Lee},
  title     = {LNEM: Lunar Neural Elevation Model},
  booktitle = {Proceedings of the IEEE/CVF Conference on Computer Vision and Pattern Recognition (CVPR)},
  year      = {2026},
}
```

Please also cite ISIS3 — see [DOI-USGS/ISIS3](https://github.com/DOI-USGS/ISIS3#citing-isis) for the latest DOI.

---

## License

Lunar Studio's own code (driver scripts, Dockerfile, examples) is released under the [MIT License](LICENSE). The bundled ISIS3 binaries retain the upstream [ISIS3 public-domain notice](https://github.com/DOI-USGS/ISIS3/blob/dev/LICENSE.md); the bundled Ames Stereo Pipeline retains its [Apache 2.0 license](https://github.com/NeoGeographyToolkit/StereoPipeline/blob/master/LICENSE).
