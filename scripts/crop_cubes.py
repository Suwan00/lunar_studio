#!/usr/bin/env python3
"""
Lunar Studio — site cropping tool (Step 5 of the preprocessing pipeline).

Given a reference ISIS cube and a list of *site* centre pixels (sample, line),
crops a fixed-size (default 512x512) window around each centre on the
reference cube, then uses `campt` / `camrange` to find the matching physical
ground patch on every other image and crops each to that same footprint.

Usage (CSV-driven, recommended for multi-site runs):

    python3 crop_cubes.py \\
        --working-dir /workspace/<region> \\
        --reference M1114512054LE \\
        --others M1114497847LE M1114504950LE \\
        --sites-csv sites.csv \\
        --crop-size 512

where `sites.csv` has a header row and one row per site:

    site_id,sample,line
    1,2532,40047
    2,2258,34102

Usage (ad-hoc single-site, no CSV):

    python3 crop_cubes.py \\
        --working-dir /workspace/<region> \\
        --reference M1114512054LE \\
        --others M1114497847LE M1114504950LE \\
        --site-id 1 --sample 2532 --line 40047

Input cubes are expected to end in ``.echo.cub`` (i.e. produced by
`lronacecho` in Stage 4 of the pipeline). Output cubes land under
``<working-dir>/site<id>/<basename>.echo.site<id>.cub`` together with a
stretched grayscale PNG preview, ready for the site-mode `findfeatures` +
`jigsaw` loop in ``process_cubes.sh --mode site``.
"""

from __future__ import annotations

import argparse
import csv
import os
import re
import subprocess
import sys
from dataclasses import dataclass
from pathlib import Path
from typing import Dict, List, Optional


def run_command(cmd: str) -> str:
    """Execute an ISIS3 command and return stdout; prints stderr on failure."""
    print(f"  $ {cmd}")
    proc = subprocess.Popen(
        cmd,
        shell=True,
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        universal_newlines=True,
    )
    stdout, stderr = proc.communicate()
    if proc.returncode != 0:
        print(f"  ! command failed (exit {proc.returncode}): {cmd}")
        if stderr:
            print(f"  ! stderr: {stderr.strip()}")
        return ""
    return stdout


def extract_latlon_bounds(camrange_output: str) -> Dict[str, float]:
    """Pull the UniversalGroundRange bounding box out of `camrange` stdout."""
    pattern = (
        r"Group = UniversalGroundRange.*?"
        r"MinimumLatitude\s+=\s+(\S+).*?"
        r"MaximumLatitude\s+=\s+(\S+).*?"
        r"MinimumLongitude\s+=\s+(\S+).*?"
        r"MaximumLongitude\s+=\s+(\S+)"
    )
    match = re.search(pattern, camrange_output, re.DOTALL)
    if not match:
        return {}
    return {
        "min_lat": float(match.group(1)),
        "max_lat": float(match.group(2)),
        "min_lon": float(match.group(3)),
        "max_lon": float(match.group(4)),
    }


def extract_sample_line(campt_output: str) -> Dict[str, float]:
    """Pull Sample/Line from the output of `campt type=ground`."""
    s_match = re.search(r"Sample\s+=\s+(\S+)", campt_output)
    l_match = re.search(r"Line\s+=\s+(\S+)", campt_output)
    if not (s_match and l_match):
        return {}
    return {"sample": float(s_match.group(1)), "line": float(l_match.group(1))}


def get_pixel_minmax(cub_file: Path) -> Dict[str, float]:
    """Probe a cube's dynamic range via `isis2std`'s InputMinimum/InputMaximum
    report so we can stretch the PNG preview to the full-image range rather
    than the tiny crop."""
    tmp_png = cub_file.with_suffix(".tmp.png")
    output = run_command(f"isis2std from={cub_file} to={tmp_png} format=png")
    if tmp_png.exists():
        tmp_png.unlink()
    min_m = re.search(r"InputMinimum\s+=\s+(\S+)", output)
    max_m = re.search(r"InputMaximum\s+=\s+(\S+)", output)
    if min_m and max_m:
        return {"min": float(min_m.group(1)), "max": float(max_m.group(1))}
    return {}


@dataclass(frozen=True)
class Site:
    site_id: int
    sample: int
    line: int


def load_sites_from_csv(path: Path) -> List[Site]:
    """Load a `sites.csv` with columns site_id,sample,line (header required)."""
    with path.open() as f:
        reader = csv.DictReader(f)
        required = {"site_id", "sample", "line"}
        missing = required.difference(reader.fieldnames or [])
        if missing:
            raise ValueError(
                f"{path}: missing required column(s) {sorted(missing)}; "
                f"saw {reader.fieldnames}"
            )
        sites = [
            Site(
                site_id=int(row["site_id"]),
                sample=int(float(row["sample"])),
                line=int(float(row["line"])),
            )
            for row in reader
        ]
    if not sites:
        raise ValueError(f"{path}: no site rows found.")
    return sites


def resolve_cube(working_dir: Path, product_id: str) -> Optional[Path]:
    """Resolve <product_id>*.echo.cub under working_dir (first match wins)."""
    matches = sorted(working_dir.glob(f"{product_id}*.echo.cub"))
    return matches[0] if matches else None


def crop_reference(
    reference_cub: Path,
    site: Site,
    crop_size: int,
    site_dir: Path,
) -> Optional[Path]:
    """Crop the reference cube at (sample, line) centre; return output path."""
    half = crop_size // 2
    crop_sample = site.sample - half
    crop_line = site.line - half
    base = reference_cub.name.replace(".echo.cub", "")
    out_cub = site_dir / f"{base}.echo.site{site.site_id}.cub"

    if out_cub.exists():
        print(f"  [reference] already cropped -> {out_cub.name}")
        return out_cub

    print(
        f"  [reference] crop centre=({site.sample},{site.line}) -> "
        f"start=({crop_sample},{crop_line}), size={crop_size}x{crop_size}"
    )
    run_command(
        f"crop from={reference_cub} to={out_cub} "
        f"sample={crop_sample} line={crop_line} "
        f"nsamples={crop_size} nlines={crop_size}"
    )
    return out_cub if out_cub.exists() else None


def crop_other(
    other_cub: Path,
    bounds: Dict[str, float],
    site: Site,
    crop_size: int,
    site_dir: Path,
) -> Optional[Path]:
    """Find where the reference's ground bounds land on `other_cub`, centre
    a crop window there, and emit the cropped cube."""
    base = other_cub.name.replace(".echo.cub", "")
    out_cub = site_dir / f"{base}.echo.site{site.site_id}.cub"
    if out_cub.exists():
        print(f"  [{base}] already cropped -> {out_cub.name}")
        return out_cub

    min_res = extract_sample_line(
        run_command(
            f"campt from={other_cub} type=ground "
            f"lat={bounds['min_lat']} lon={bounds['min_lon']}"
        )
    )
    max_res = extract_sample_line(
        run_command(
            f"campt from={other_cub} type=ground "
            f"lat={bounds['max_lat']} lon={bounds['max_lon']}"
        )
    )
    if not (min_res and max_res):
        print(f"  [{base}] failed to resolve lat/lon, skipping")
        return None

    centre_sample = int((min_res["sample"] + max_res["sample"]) / 2)
    centre_line = int((min_res["line"] + max_res["line"]) / 2)
    half = crop_size // 2
    crop_sample = centre_sample - half
    crop_line = centre_line - half
    print(
        f"  [{base}] centre=({centre_sample},{centre_line}) -> "
        f"start=({crop_sample},{crop_line})"
    )
    run_command(
        f"crop from={other_cub} to={out_cub} "
        f"sample={crop_sample} line={crop_line} "
        f"nsamples={crop_size} nlines={crop_size}"
    )
    return out_cub if out_cub.exists() else None


def write_png(cub_file: Path, stretch_source: Optional[Path]) -> Optional[Path]:
    """Write a grayscale PNG preview; stretched to the dynamic range of
    `stretch_source` (typically the full-size original) when provided."""
    png = cub_file.with_suffix(".png")
    if png.exists():
        return png
    pix = get_pixel_minmax(stretch_source) if stretch_source is not None else {}
    if pix:
        cmd = (
            f"isis2std from={cub_file} to={png} format=png "
            f"stretch=MANUAL minimum={pix['min']} maximum={pix['max']}"
        )
    else:
        cmd = (
            f"isis2std from={cub_file} to={png} format=png "
            f"minpercent=0.1 maxpercent=99.9"
        )
    run_command(cmd)
    return png if png.exists() else None


def process_site(
    working_dir: Path,
    reference_id: str,
    other_ids: List[str],
    site: Site,
    crop_size: int,
) -> bool:
    print(f"\n{'=' * 60}\nSite {site.site_id}\n{'=' * 60}")
    site_dir = working_dir / f"site{site.site_id}"
    site_dir.mkdir(exist_ok=True)

    reference_cub = resolve_cube(working_dir, reference_id)
    if reference_cub is None:
        print(
            f"  ERROR: reference cube {reference_id}*.echo.cub not found in "
            f"{working_dir}"
        )
        return False

    ref_cropped = crop_reference(reference_cub, site, crop_size, site_dir)
    if ref_cropped is None:
        print("  ERROR: failed to crop reference")
        return False
    write_png(ref_cropped, stretch_source=reference_cub)

    bounds = extract_latlon_bounds(run_command(f"camrange from={ref_cropped}"))
    if not bounds:
        print("  ERROR: could not read lat/lon range from cropped reference")
        return False
    print(
        f"  site bounds: "
        f"lat[{bounds['min_lat']:.6f}, {bounds['max_lat']:.6f}]  "
        f"lon[{bounds['min_lon']:.6f}, {bounds['max_lon']:.6f}]"
    )

    all_ok = True
    for other_id in other_ids:
        other_cub = resolve_cube(working_dir, other_id)
        if other_cub is None:
            print(f"  [{other_id}] cube not found, skipping")
            all_ok = False
            continue
        cropped = crop_other(other_cub, bounds, site, crop_size, site_dir)
        if cropped is None:
            all_ok = False
            continue
        write_png(cropped, stretch_source=other_cub)
    return all_ok


def main() -> int:
    parser = argparse.ArgumentParser(
        description=(
            "Lunar Studio — site cropping (Step 5 of the preprocessing "
            "pipeline)."
        ),
        formatter_class=argparse.RawDescriptionHelpFormatter,
    )
    parser.add_argument(
        "--working-dir", type=Path, required=True,
        help="Directory containing the *.echo.cub inputs (and where site<N>/ "
             "output folders will be written).",
    )
    parser.add_argument(
        "--reference", required=True,
        help="Reference image product ID (prefix; matched against "
             "<id>*.echo.cub).",
    )
    parser.add_argument(
        "--others", nargs="+", default=[],
        help="Other image product IDs to co-crop (prefixes).",
    )
    parser.add_argument(
        "--crop-size", type=int, default=512,
        help="Square crop size in pixels (default: 512).",
    )

    source = parser.add_mutually_exclusive_group(required=True)
    source.add_argument(
        "--sites-csv", type=Path,
        help="CSV with header 'site_id,sample,line' (one row per site).",
    )
    source.add_argument(
        "--site-id", type=int,
        help="Ad-hoc single-site mode — numeric site id "
             "(requires --sample and --line).",
    )
    parser.add_argument(
        "--sample", type=int,
        help="Centre sample pixel (used with --site-id).",
    )
    parser.add_argument(
        "--line", type=int,
        help="Centre line pixel (used with --site-id).",
    )

    args = parser.parse_args()

    if args.site_id is not None:
        if args.sample is None or args.line is None:
            parser.error("--site-id requires both --sample and --line.")
        sites = [Site(site_id=args.site_id, sample=args.sample, line=args.line)]
    else:
        sites = load_sites_from_csv(args.sites_csv)

    working_dir = args.working_dir.resolve()
    if not working_dir.is_dir():
        print(f"ERROR: working-dir not found: {working_dir}", file=sys.stderr)
        return 1

    print("Lunar Studio — crop_cubes.py")
    print(f"  working directory : {working_dir}")
    print(f"  reference         : {args.reference}")
    print(f"  others            : "
          f"{', '.join(args.others) if args.others else '(none)'}")
    print(f"  crop size         : {args.crop_size}x{args.crop_size}")
    print(f"  sites             : {[s.site_id for s in sites]}")

    succeeded = 0
    failed = 0
    for site in sites:
        ok = process_site(
            working_dir, args.reference, args.others, site, args.crop_size,
        )
        if ok:
            succeeded += 1
        else:
            failed += 1

    print(f"\n{'=' * 60}")
    print(f"Summary: {succeeded} site(s) succeeded, {failed} failed.")
    print(f"{'=' * 60}")
    return 0 if failed == 0 else 2


if __name__ == "__main__":
    sys.exit(main())
