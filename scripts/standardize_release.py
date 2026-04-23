#!/usr/bin/env python3
"""
Lunar Studio — release dataset file-name standardiser.

Takes a directory tree produced by the csv2parquet pipeline (messy,
with ``.echo.site<N>.pixelvalue.png`` / ``.slantdistance.png`` /
``_pointcloud.{ply,xyz,xyzrgb}`` / ``*.zdepth_plot.png`` etc.) and
produces a clean release tree with only:

  <outdir>/<mission>/<dem>/<region>/
        <reference_id>.cub                            # full cube
        jigsawErr_bundleout_points.csv                # jigsaw result
        site<N>/
            <cube_id>.site<N>.png                     # 1 per orbit
            <cube_id>.site<N>_zdepth.parquet          # 1 per orbit
            <cube_id>.site<N>_lola_cub_3d_comparison_{lola_3d,cub_3d,pixel_coords}.npy

``.echo.`` and ``.pixelvalue.`` infixes are dropped. Pre-existing npy
files that still carry the ``.echo.`` prefix are renamed in place.

Typical invocation::

    python3 standardize_release.py \\
        --config release_config.yaml \\
        --source-root /ssd00/.../2026cvpr \\
        --release-root /ssd00/.../2026cvpr/release_data

Per-region entries in the config carry everything this script needs
(see extract_site_bounds.py docstring for the full schema, plus the
optional keys below):

    regions:
      - mission: lro_nac
        dem: nacdtm
        region: apollo15
        reference_id: M1144779525RE
        num_sites: 5
        site_cubes_dir: /.../nacdtm/apollo15
        full_cube: /.../nacdtm/apollo15/M1144779525RE.echo.cub  # optional
        jigsaw_csv: /.../nacdtm/apollo15/jigsawErr_bundleout_points.csv  # optional
        cube_ids: [M1144779525RE, M1356476581RE, M183504057LE]  # orbits to ship
"""

from __future__ import annotations

import argparse
import json
import re
import shutil
import sys
from pathlib import Path
from typing import Dict, Iterable, List, Optional

try:
    import yaml
except ImportError:
    yaml = None


def load_config(path: Path) -> Dict:
    text = path.read_text()
    if path.suffix.lower() in (".yaml", ".yml"):
        if yaml is None:
            raise RuntimeError(
                "PyYAML is not installed; run `pip install pyyaml`"
            )
        return yaml.safe_load(text)
    return json.loads(text)


def copy_if_missing(src: Path, dst: Path) -> bool:
    """Copy ``src`` to ``dst`` unless ``dst`` already exists with the
    same content-size heuristic. Returns True if a copy happened."""
    if dst.exists() and dst.stat().st_size == src.stat().st_size:
        return False
    dst.parent.mkdir(parents=True, exist_ok=True)
    shutil.copy2(src, dst)
    return True


def rename_existing_npy(site_dir: Path, cube_id: str, site: int) -> int:
    """Rename pre-existing ``*.echo.site<N>_lola_cub_3d_comparison_*.npy``
    files in ``site_dir`` to drop the ``.echo.`` infix."""
    renamed = 0
    for old in site_dir.glob(f"{cube_id}.echo.site{site}_lola_cub_3d_comparison_*.npy"):
        new = site_dir / old.name.replace(".echo.", ".")
        if new != old and not new.exists():
            old.rename(new)
            renamed += 1
    return renamed


def standardise_site(source_site_dir: Path,
                     release_site_dir: Path,
                     cube_ids: Iterable[str],
                     site: int) -> Dict[str, int]:
    """For each orbit's output produced by csv2parquet, extract the
    parquet + pixelvalue PNG and rename into the release layout."""
    release_site_dir.mkdir(parents=True, exist_ok=True)

    stats = {"parquet": 0, "png": 0, "npy_renamed": 0, "missing": 0}
    output_dir = source_site_dir / "output"
    if not output_dir.is_dir():
        # Some pipelines leave the artifacts directly in the site folder.
        output_dir = source_site_dir

    for cube_id in cube_ids:
        old_pq = output_dir / f"{cube_id}.echo.site{site}_zdepth.parquet"
        old_png = output_dir / f"{cube_id}.echo.site{site}.pixelvalue.png"
        # Some csv2parquet runs used the `.echo.site{N}.zdepth.parquet` dot
        # variant; fall back to that.
        if not old_pq.exists():
            alt = output_dir / f"{cube_id}.echo.site{site}.zdepth.parquet"
            if alt.exists():
                old_pq = alt
        if not old_png.exists():
            alt = output_dir / f"{cube_id}.echo.site{site}.pho.campt.png"
            if alt.exists():
                old_png = alt

        new_pq = release_site_dir / f"{cube_id}.site{site}_zdepth.parquet"
        new_png = release_site_dir / f"{cube_id}.site{site}.png"

        if old_pq.exists():
            copy_if_missing(old_pq, new_pq)
            stats["parquet"] += 1
        else:
            print(f"  ! missing parquet for {cube_id} site{site}: {old_pq}")
            stats["missing"] += 1

        if old_png.exists():
            copy_if_missing(old_png, new_png)
            stats["png"] += 1
        else:
            print(f"  ! missing pixelvalue png for {cube_id} site{site}")
            stats["missing"] += 1

        stats["npy_renamed"] += rename_existing_npy(
            release_site_dir, cube_id, site,
        )
    return stats


def standardise_region_level(region: Dict, release_region_dir: Path) -> None:
    """Copy the full (uncropped) reference cube + jigsaw output to the
    region-level release folder, dropping the ``.echo.`` infix."""
    release_region_dir.mkdir(parents=True, exist_ok=True)
    reference_id = region["reference_id"]

    full_cube = region.get("full_cube")
    if full_cube:
        src = Path(full_cube)
        dst = release_region_dir / f"{reference_id}.cub"
        if src.exists():
            if copy_if_missing(src, dst):
                print(f"  copied full cube: {src} -> {dst.name}")
            else:
                print(f"  skip full cube (up-to-date): {dst.name}")
        else:
            print(f"  ! full_cube missing: {src}")

    jigsaw_csv = region.get("jigsaw_csv")
    if jigsaw_csv:
        src = Path(jigsaw_csv)
        dst = release_region_dir / "jigsawErr_bundleout_points.csv"
        if src.exists():
            if copy_if_missing(src, dst):
                print(f"  copied jigsaw csv: {src.name} -> {dst.name}")
        else:
            print(f"  ! jigsaw_csv missing: {src}")


def process_region(region: Dict, source_root: Path,
                   release_root: Path) -> Dict:
    mission = region["mission"]
    dem = region["dem"]
    region_name = region["region"]
    reference_id = region["reference_id"]
    num_sites = int(region["num_sites"])
    site_cubes_dir = Path(region["site_cubes_dir"])
    cube_ids = region.get("cube_ids") or [reference_id]

    release_region_dir = release_root / mission / dem / region_name
    print(f"\n=== {mission}/{dem}/{region_name} (reference={reference_id}, "
          f"{num_sites} sites) ===")

    standardise_region_level(region, release_region_dir)

    totals = {"parquet": 0, "png": 0, "npy_renamed": 0, "missing": 0}
    for n in range(1, num_sites + 1):
        source_site = site_cubes_dir / f"site{n}"
        release_site = release_region_dir / f"site{n}"
        print(f"  site{n}:")
        stats = standardise_site(source_site, release_site, cube_ids, n)
        for k, v in stats.items():
            totals[k] += v
        print(f"    parquet={stats['parquet']} png={stats['png']} "
              f"npy_renamed={stats['npy_renamed']} missing={stats['missing']}")

    return {
        "mission": mission, "dem": dem, "region": region_name,
        "totals": totals,
    }


def main() -> int:
    parser = argparse.ArgumentParser(
        description=__doc__.split("\n\n")[0],
        formatter_class=argparse.RawDescriptionHelpFormatter,
    )
    parser.add_argument("--config", type=Path, required=True,
                        help="Release config (YAML or JSON).")
    parser.add_argument("--source-root", type=Path, required=True,
                        help="Directory under which per-region cubes/outputs "
                             "live (informational; paths in config are "
                             "absolute).")
    parser.add_argument("--release-root", type=Path, required=True,
                        help="Destination root for the release dataset "
                             "(mission/dem/region/site tree).")
    args = parser.parse_args()

    cfg = load_config(args.config)
    if "regions" not in cfg or not isinstance(cfg["regions"], list):
        print("ERROR: config must contain a top-level 'regions' list",
              file=sys.stderr)
        return 1

    summary = [process_region(r, args.source_root, args.release_root)
               for r in cfg["regions"]]
    print("\n" + "=" * 60)
    print("Standardisation summary")
    print("=" * 60)
    for s in summary:
        t = s["totals"]
        print(f"  {s['mission']}/{s['dem']}/{s['region']:<15} "
              f"parquet={t['parquet']:>3}  png={t['png']:>3}  "
              f"npy_renamed={t['npy_renamed']:>3}  missing={t['missing']:>3}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
