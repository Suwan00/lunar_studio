#!/usr/bin/env python3
"""
Lunar Studio — per-site lat/lon bounds extractor.

For each site of each region listed in the release config, run
``camrange`` on the site's *reference* cube and emit a JSON file
summarising the four-corner bounding box. The JSON is what you
hand to the LOLA PDS search UI to pull the ``RDR_*_PointPerRow_csv_table.csv``
files that cover each site.

Typical invocation:

    python3 extract_site_bounds.py \\
        --config release_config.yaml \\
        --output /tmp/site_bounds.json

``release_config.yaml`` example::

    regions:
      - mission: lro_nac
        dem: nacdtm
        region: apollo15
        reference_id: M1144779525RE     # cube that defined the site crops
        num_sites: 5
        site_cubes_dir: /ssd00/.../nacdtm/apollo15
        cube_suffix: ".echo.site{site}.cub"    # how the per-site cube is named

The tool runs ``camrange from=<site_cubes_dir>/site<N>/<reference_id><cube_suffix>``
for each site. The cube has to already exist (i.e. the site crop must
have been produced by ``crop_cubes.py``).
"""

from __future__ import annotations

import argparse
import json
import re
import subprocess
import sys
from pathlib import Path
from typing import Dict, List, Optional

try:
    import yaml
except ImportError:
    yaml = None


CAMRANGE_PATTERN = re.compile(
    r"Group\s*=\s*UniversalGroundRange.*?"
    r"MinimumLatitude\s+=\s+(\S+).*?"
    r"MaximumLatitude\s+=\s+(\S+).*?"
    r"MinimumLongitude\s+=\s+(\S+).*?"
    r"MaximumLongitude\s+=\s+(\S+)",
    re.DOTALL,
)


def run_camrange(cube_path: Path) -> Optional[Dict[str, float]]:
    """Execute ``camrange`` and return the ground bounding box."""
    proc = subprocess.run(
        ["camrange", f"from={cube_path}"],
        capture_output=True, text=True,
    )
    if proc.returncode != 0:
        print(f"  ! camrange failed on {cube_path.name}: "
              f"{proc.stderr.strip().splitlines()[-1] if proc.stderr else '?'}",
              file=sys.stderr)
        return None
    m = CAMRANGE_PATTERN.search(proc.stdout)
    if not m:
        print(f"  ! UniversalGroundRange not found in camrange output for "
              f"{cube_path.name}", file=sys.stderr)
        return None
    return {
        "min_lat": float(m.group(1)),
        "max_lat": float(m.group(2)),
        "min_lon": float(m.group(3)),
        "max_lon": float(m.group(4)),
    }


def load_config(path: Path) -> Dict:
    text = path.read_text()
    if path.suffix.lower() in (".yaml", ".yml"):
        if yaml is None:
            raise RuntimeError(
                "PyYAML is not installed; run `pip install pyyaml` or "
                "use a JSON config."
            )
        return yaml.safe_load(text)
    return json.loads(text)


def process_region(region: Dict) -> Dict:
    mission = region["mission"]
    dem = region["dem"]
    region_name = region["region"]
    reference_id = region["reference_id"]
    num_sites = int(region["num_sites"])
    site_cubes_dir = Path(region["site_cubes_dir"])
    cube_suffix = region.get("cube_suffix", ".echo.site{site}.cub")

    print(f"\n=== {mission}/{dem}/{region_name} "
          f"(reference={reference_id}, {num_sites} sites) ===")

    result: Dict[str, Dict] = {}
    for n in range(1, num_sites + 1):
        suffix = cube_suffix.format(site=n)
        cube = site_cubes_dir / f"site{n}" / f"{reference_id}{suffix}"
        if not cube.exists():
            print(f"  site{n}: cube missing — {cube}")
            result[f"site{n}"] = {"error": f"cube not found: {cube}"}
            continue
        print(f"  site{n}: camrange {cube.name}")
        bounds = run_camrange(cube)
        if bounds is None:
            result[f"site{n}"] = {"error": "camrange failed"}
            continue
        print(f"    lat[{bounds['min_lat']:.5f}, {bounds['max_lat']:.5f}]  "
              f"lon[{bounds['min_lon']:.5f}, {bounds['max_lon']:.5f}]")
        result[f"site{n}"] = {
            "reference_cube": str(cube),
            "reference_id": reference_id,
            **bounds,
        }
    return {
        "mission": mission,
        "dem": dem,
        "region": region_name,
        "sites": result,
    }


def main() -> int:
    parser = argparse.ArgumentParser(
        description=__doc__.split("\n\n")[0],
        formatter_class=argparse.RawDescriptionHelpFormatter,
    )
    parser.add_argument("--config", type=Path, required=True,
                        help="Release config (YAML or JSON).")
    parser.add_argument("--output", type=Path, required=True,
                        help="Destination JSON summarising all site bounds.")
    args = parser.parse_args()

    cfg = load_config(args.config)
    if "regions" not in cfg or not isinstance(cfg["regions"], list):
        print("ERROR: config must contain a top-level 'regions' list",
              file=sys.stderr)
        return 1

    summary: List[Dict] = []
    for region in cfg["regions"]:
        summary.append(process_region(region))

    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(json.dumps(summary, indent=2, ensure_ascii=False))
    print(f"\n✓ wrote {args.output}")
    print(f"  Use these bounds as the lat/lon search window in the")
    print(f"  LOLA PDS Pds3PointPerRow search "
          f"(https://imbrium.mit.edu/EXTRAS/LOLA_LOCAL/) to download the")
    print(f"  matching RDR_*_PointPerRow_csv_table.csv per site.")
    return 0


if __name__ == "__main__":
    sys.exit(main())
