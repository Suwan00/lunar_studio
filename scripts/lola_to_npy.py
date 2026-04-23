#!/usr/bin/env python3
"""
Lunar Studio — LOLA CSV → npy triplet converter.

For a single site, given:

  * a reference cube (``<reference_id>.site<N>.cub`` after the release
    rename, or ``.echo.site<N>.cub`` pre-rename), and
  * a LOLA PDS ``RDR_*_PointPerRow_csv_table.csv`` covering the site's
    lat/lon bounds,

projects each LOLA point into the cube via ``campt type=ground`` and
emits the three ``.npy`` arrays used for LNEM-style evaluation:

  <cube_id>.site<N>_lola_cub_3d_comparison_lola_3d.npy       float64 (N, 3)  MOON_ME km
  <cube_id>.site<N>_lola_cub_3d_comparison_cub_3d.npy        float64 (N, 3)  MOON_ME km
  <cube_id>.site<N>_lola_cub_3d_comparison_pixel_coords.npy  float64 (N, 2)  (sample, line)

Only LOLA points whose (sample, line) falls inside the cube's extent
are retained. The standardised, renamed LOLA CSV is also written
alongside as ``<region>_site<N>_<cube_id>.<orig_csv_name>``.

Typical invocation::

    python3 lola_to_npy.py \\
        --cube  release_data/lro_nac/nacdtm/apollo15/site1/M1144779525RE.site1.cub \\
        --lola  /path/to/RDR_3E3E_26N26NPointPerRow_csv_table.csv \\
        --cube-id M1144779525RE \\
        --site 1 \\
        --region apollo15 \\
        --outdir release_data/lro_nac/nacdtm/apollo15/site1/
"""

from __future__ import annotations

import argparse
import csv
import os
import re
import shutil
import subprocess
import sys
from pathlib import Path
from typing import Optional, Tuple

import numpy as np


SAMPLE_RE = re.compile(r"Sample\s+=\s+(\S+)")
LINE_RE = re.compile(r"Line\s+=\s+(\S+)")
BF_RE = re.compile(r"BodyFixedCoordinate\s+=\s+\(([^)]+)\)")


def lola_latlon_radius_to_xyz(lat_deg: np.ndarray, lon_deg: np.ndarray,
                              radius_km: np.ndarray) -> np.ndarray:
    """Spherical → MOON_ME Cartesian (same convention as ISIS
    BodyFixedCoordinate, in kilometres)."""
    lat = np.deg2rad(lat_deg)
    lon = np.deg2rad(lon_deg)
    x = radius_km * np.cos(lat) * np.cos(lon)
    y = radius_km * np.cos(lat) * np.sin(lon)
    z = radius_km * np.sin(lat)
    return np.stack([x, y, z], axis=-1)


def campt_ground(cube: Path, lat: float, lon: float) -> Optional[dict]:
    """Run ``campt type=ground`` for a single (lat, lon) and parse
    the sample, line and BodyFixedCoordinate from stdout."""
    proc = subprocess.run(
        ["campt", f"from={cube}", "type=ground",
         f"lat={lat}", f"lon={lon}", "allowerror=true"],
        capture_output=True, text=True,
    )
    if proc.returncode != 0:
        return None
    out = proc.stdout
    sm = SAMPLE_RE.search(out)
    lm = LINE_RE.search(out)
    bm = BF_RE.search(out)
    if not (sm and lm and bm):
        return None
    try:
        bf = [float(v.strip()) for v in bm.group(1).split(",")]
    except ValueError:
        return None
    if len(bf) != 3:
        return None
    return {
        "sample": float(sm.group(1)),
        "line": float(lm.group(1)),
        "body_fixed": bf,
    }


def cube_dimensions(cube: Path) -> Tuple[int, int]:
    """Grab sample/line counts from ``catlab`` / ``isisheader`` output."""
    # Use `catlab` (prints PVL label); extract SampleCount / LineCount.
    proc = subprocess.run(
        ["catlab", f"from={cube}"], capture_output=True, text=True,
    )
    if proc.returncode != 0:
        raise RuntimeError(f"catlab failed on {cube}")
    m_s = re.search(r"Samples\s*=\s*(\d+)", proc.stdout)
    m_l = re.search(r"Lines\s*=\s*(\d+)", proc.stdout)
    if not (m_s and m_l):
        raise RuntimeError(f"Cannot parse dimensions from catlab on {cube}")
    return int(m_s.group(1)), int(m_l.group(1))


def load_lola_csv(path: Path) -> Tuple[np.ndarray, np.ndarray, np.ndarray]:
    """Read a LOLA ``RDR_*_PointPerRow_csv_table.csv`` and return
    (lat, lon, radius_km) arrays."""
    # The column layout varies slightly between LOLA products; we key
    # on column names so extra columns don't break us.
    with path.open() as f:
        reader = csv.DictReader(f, skipinitialspace=True)
        fields = [f.strip() for f in (reader.fieldnames or [])]
        reader.fieldnames = fields
        lats, lons, radii = [], [], []
        for row in reader:
            try:
                lats.append(float(row["Pt_Latitude"]))
                lons.append(float(row["Pt_Longitude"]))
                radii.append(float(row["Pt_Radius"]))
            except (KeyError, ValueError):
                continue
    return np.array(lats), np.array(lons), np.array(radii)


def main() -> int:
    parser = argparse.ArgumentParser(
        description=__doc__.split("\n\n")[0],
        formatter_class=argparse.RawDescriptionHelpFormatter,
    )
    parser.add_argument("--cube", type=Path, required=True,
                        help="Reference cube for the site.")
    parser.add_argument("--lola", type=Path, required=True,
                        help="Raw LOLA PDS CSV (RDR_*_PointPerRow_csv_table.csv).")
    parser.add_argument("--cube-id", required=True,
                        help="Cube product id (e.g. M1144779525RE). Used in "
                             "output filenames.")
    parser.add_argument("--site", type=int, required=True,
                        help="Site id (integer).")
    parser.add_argument("--region", required=True,
                        help="Region label (e.g. apollo15). Used in the "
                             "renamed LOLA CSV filename.")
    parser.add_argument("--outdir", type=Path, required=True,
                        help="Destination directory for the npy triplet "
                             "and the renamed LOLA CSV.")
    args = parser.parse_args()

    if not args.cube.exists():
        print(f"ERROR: cube {args.cube} not found", file=sys.stderr)
        return 1
    if not args.lola.exists():
        print(f"ERROR: lola csv {args.lola} not found", file=sys.stderr)
        return 1
    args.outdir.mkdir(parents=True, exist_ok=True)

    print(f"LOLA: {args.lola.name}")
    lats, lons, radii = load_lola_csv(args.lola)
    print(f"  loaded {len(lats)} LOLA points")

    # Normalise longitudes to 0..360 to match ISIS default (ISIS also
    # happily accepts -180..180; campt will resolve either way).
    samples_px, lines_px = cube_dimensions(args.cube)
    print(f"  cube extent: samples={samples_px}, lines={lines_px}")

    keep_idx: list[int] = []
    cube_xyz: list[list[float]] = []
    pixel_coords: list[Tuple[float, float]] = []

    print(f"  running campt on each LOLA point — this is serial, "
          f"count={len(lats)}")
    for i in range(len(lats)):
        res = campt_ground(args.cube, lats[i], lons[i])
        if res is None:
            continue
        s, l = res["sample"], res["line"]
        if not (1 <= s <= samples_px and 1 <= l <= lines_px):
            continue
        keep_idx.append(i)
        cube_xyz.append(res["body_fixed"])
        pixel_coords.append((s, l))
        if (len(keep_idx) % 200) == 0:
            print(f"    {len(keep_idx)} kept after {i+1}/{len(lats)} points")

    if not keep_idx:
        print("  ! no LOLA points fell inside the cube — output not written",
              file=sys.stderr)
        return 2

    lola_xyz = lola_latlon_radius_to_xyz(
        lats[keep_idx], lons[keep_idx], radii[keep_idx],
    )
    cube_xyz_arr = np.asarray(cube_xyz, dtype=np.float64)
    pixel_arr = np.asarray(pixel_coords, dtype=np.float64)

    base = f"{args.cube_id}.site{args.site}_lola_cub_3d_comparison"
    np.save(args.outdir / f"{base}_lola_3d.npy", lola_xyz)
    np.save(args.outdir / f"{base}_cub_3d.npy", cube_xyz_arr)
    np.save(args.outdir / f"{base}_pixel_coords.npy", pixel_arr)

    renamed_csv = (
        args.outdir
        / f"{args.region}_site{args.site}_{args.cube_id}.{args.lola.name}"
    )
    shutil.copy2(args.lola, renamed_csv)

    print(f"\n✓ {len(keep_idx)} / {len(lats)} LOLA points projected into cube")
    print(f"  wrote {args.outdir}/{base}_{{lola_3d,cub_3d,pixel_coords}}.npy")
    print(f"  wrote {renamed_csv.name}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
