#!/usr/bin/env python3
"""
Lunar Studio — csv2parquet.py

Converts a directory of `campt` CSV outputs (one per image/site) into the
compact parquet + PNG preview + PLY/XYZ pointcloud bundle that LNEM and
similar neural renderers ingest directly.

Input
-----
Each CSV must have been produced by the Lunar Studio / ISIS3 `campt` with
the `useoffset=true format=flat` flags so that the following columns are
present:

  * ``Sample``, ``Line``                — per-pixel image coordinates
  * ``Latitude``, ``Longitude``         — planetocentric ground coordinates
  * ``LookDirectionCameraX/Y/Z``        — body-fixed ray direction
  * ``SlantDistance``                   — sensor-to-surface distance (km)
  * ``PixelValue``                      — calibrated radiance
  * ``BodyFixedCoordinateX/Y/Z``        — ground point in MOON_ME (km)

Output
------
For every input CSV (``<base>.csv``):

  * ``<base>_zdepth.parquet``   — original columns + computed ``ZDepth``
  * ``<base>.pixelvalue.png``   — grayscale preview of radiance
  * ``<base>.slantdistance.png`` / ``<base>.zdepth.png``      — turbo colormap
  * ``<base>.slantdistance_plot.png`` / ``<base>.zdepth_plot.png``
  * ``<base>_pointcloud.ply`` / ``.xyz`` / ``.xyzrgb``

Usage
-----
  # Single CSV file
  python3 csv2parquet.py --input data.csv --product-id M1121224102LE

  # All CSVs in one directory (typical site<N>/ output of crop_cubes.py)
  python3 csv2parquet.py --directory ./csv_files

  # Recurse across region directories (e.g. nacdtm/site1 ... sldem/site5)
  python3 csv2parquet.py --regions nacdtm sldem
  python3 csv2parquet.py --regions nacdtm sldem --output ./results

Version: 2.2.0
"""

import os
import re
import glob
import argparse
import numpy as np
import pandas as pd
import matplotlib.pyplot as plt
import matplotlib.cm as cm
from matplotlib.colors import Normalize
from PIL import Image
from pathlib import Path
from typing import Optional

# Configure NumPy float printing precision (affects log output only).
np.set_printoptions(precision=10, suppress=False, floatmode='fixed')

# ---------------------------------------------------------------------------
# Metadata helpers
# ---------------------------------------------------------------------------

PRODUCT_ID_RE = re.compile(r"(M\d{9,10}[LR]E)")


def parse_product_id(path: str) -> Optional[str]:
    """Extract the LROC NAC Product ID (e.g. M1121224102LE) from a filename."""
    m = PRODUCT_ID_RE.search(os.path.basename(path))
    return m.group(1) if m else None


# ---------------------------------------------------------------------------
# Step 1 — load CSV into a DataFrame
# ---------------------------------------------------------------------------

def load_csv_to_dataframe(csv_path: str) -> pd.DataFrame:
    """Load a campt CSV into a DataFrame, tolerating comma or whitespace
    delimiters."""
    print(f"  📄 Loading CSV: {csv_path}")

    try:
        df = pd.read_csv(csv_path)
        print(f"     ✓ Loaded with comma delimiter")
    except Exception:
        print(f"     ⚠ Comma delimiter failed, trying whitespace...")
        try:
            df = pd.read_csv(csv_path, delim_whitespace=True)
            print(f"     ✓ Loaded with whitespace delimiter")
        except Exception as e2:
            raise ValueError(f"Failed to load CSV: {e2}")

    # Normalise column names (strip whitespace).
    df.columns = df.columns.str.strip()

    print(f"     Shape: {df.shape}")
    print(f"     Columns: {list(df.columns)[:5]}"
          f"{'...' if len(df.columns) > 5 else ''}")
    return df


# ---------------------------------------------------------------------------
# Step 2 — compute Z-depth from look direction + slant distance
# ---------------------------------------------------------------------------

def compute_cos_from_lookdir(df: pd.DataFrame) -> np.ndarray:
    """Compute cos(theta) between the body-fixed look direction and the
    surface-normal Z axis, from LookDirectionCameraX/Y/Z."""
    x = df["LookDirectionCameraX"].to_numpy(dtype=np.float64)
    y = df["LookDirectionCameraY"].to_numpy(dtype=np.float64)
    z = df["LookDirectionCameraZ"].to_numpy(dtype=np.float64)

    n = np.sqrt(x * x + y * y + z * z, dtype=np.float64)
    n = np.where(n == 0, 1.0, n)
    cos_theta = np.divide(z, n, dtype=np.float64)

    return cos_theta


def add_zdepth_column(df: pd.DataFrame, product_id: str = None) -> pd.DataFrame:
    """Append a ``ZDepth`` column (kilometres) computed as
    SlantDistance * cos(theta) using the body-fixed look vector."""
    print(f"  🧮 Computing Z-Depth...")

    has_lookdir = all(c in df.columns for c in
                      ["LookDirectionCameraX",
                       "LookDirectionCameraY",
                       "LookDirectionCameraZ"])
    if not has_lookdir:
        raise ValueError(
            "LookDirection columns (LookDirectionCameraX/Y/Z) not found in "
            "CSV.\n"
            "This pipeline requires campt CSV output with LookDirection "
            "data.\n"
            "Please ensure your CSV was generated with campt and includes "
            "camera look vectors."
        )

    # campt always reports SlantDistance in km.
    R_km = df["SlantDistance"].to_numpy(dtype=np.float64)
    print(f"     SlantDistance range: "
          f"[{R_km.min():.6f}, {R_km.max():.6f}] km")

    cos_theta = compute_cos_from_lookdir(df)
    print(f"     cos(theta) range: "
          f"[{cos_theta.min():.10f}, {cos_theta.max():.10f}]")

    # Compute in metres for precision, then convert back to km for storage.
    R_m = R_km * 1000.0
    z_m = np.multiply(R_m, cos_theta, dtype=np.float64)
    z_km = z_m / 1000.0

    df["ZDepth"] = z_km.astype(np.float64)
    df["cos_theta"] = cos_theta.astype(np.float64)

    print(f"     ZDepth range: [{z_km.min():.10f}, {z_km.max():.10f}] km")
    print(f"     ✓ Z-Depth computed")
    return df


# ---------------------------------------------------------------------------
# Step 3 — render depth map previews
# ---------------------------------------------------------------------------

def create_depth_map_image(df: pd.DataFrame, value_column: str,
                           output_path: str, cmap='turbo', mode='RGB'):
    """Rasterise a per-pixel `value_column` into a 512x512 PNG."""
    lines = df["Line"].to_numpy(dtype=int)
    samples = df["Sample"].to_numpy(dtype=int)
    values = df[value_column].to_numpy(dtype=np.float64)

    depth_map = np.full((512, 512), np.nan)
    depth_map[lines - 1, samples - 1] = values

    valid_values = values[~np.isnan(values)]
    if len(valid_values) == 0:
        print(f"     ⚠ All values are NaN for {value_column}")
        return

    vmin, vmax = valid_values.min(), valid_values.max()

    if mode == 'L':  # Grayscale
        depth_norm = (depth_map - vmin) / (vmax - vmin)
        depth_norm[np.isnan(depth_norm)] = 1.0  # NaN -> white
        depth_img = (depth_norm * 255).astype(np.uint8)
        Image.fromarray(depth_img, mode='L').save(output_path)
    else:  # RGB with colormap
        depth_norm = (depth_map - vmin) / (vmax - vmin)
        depth_norm_filled = np.nan_to_num(depth_norm, nan=0.0)

        if hasattr(plt.colormaps, 'get_cmap'):
            colormap = plt.colormaps.get_cmap(cmap)
        else:
            colormap = cm.get_cmap(cmap)
        depth_colored = colormap(depth_norm_filled)
        depth_img = (depth_colored[:, :, :3] * 255).astype(np.uint8)

        # NaN -> white
        nan_mask = np.isnan(depth_map)
        depth_img[nan_mask] = 255

        Image.fromarray(depth_img, mode='RGB').save(output_path)

    print(f"     ✓ Saved: {output_path}")


def create_depth_map_plot(df: pd.DataFrame, value_column: str,
                          output_path: str, title: str, unit: str = "km"):
    """Matplotlib render of the same depth map with a colourbar annotation."""
    lines = df["Line"].to_numpy(dtype=int)
    samples = df["Sample"].to_numpy(dtype=int)
    values = df[value_column].to_numpy(dtype=np.float64)

    depth_map = np.full((512, 512), np.nan)
    depth_map[lines - 1, samples - 1] = values

    valid_values = values[~np.isnan(values)]
    if len(valid_values) == 0:
        return

    vmin, vmax = valid_values.min(), valid_values.max()

    fig, ax = plt.subplots(1, 1, figsize=(10, 8))
    norm = Normalize(vmin=vmin, vmax=vmax)

    # Mask NaNs so they show as background.
    masked_map = np.ma.masked_invalid(depth_map)

    im = ax.imshow(masked_map, cmap='turbo', norm=norm, origin='upper')
    cbar = fig.colorbar(im, ax=ax, fraction=0.046, pad=0.04)
    cbar.set_label(f"{value_column} ({unit})", rotation=270, labelpad=20)

    ax.set_title(f"{title}\nRange: {vmin:.6f} ~ {vmax:.6f} {unit}")
    ax.set_xlabel("Sample")
    ax.set_ylabel("Line")

    plt.tight_layout()
    plt.savefig(output_path, dpi=150, bbox_inches='tight')
    plt.close()

    print(f"     ✓ Saved: {output_path}")


# ---------------------------------------------------------------------------
# Step 4 — build a pointcloud from the body-fixed coordinates
# ---------------------------------------------------------------------------

def create_pointcloud(df: pd.DataFrame, output_base: str,
                      product_id: str = "unknown"):
    """Emit .ply / .xyz / .xyzrgb pointclouds from BodyFixedCoordinate columns
    (MOON_ME frame, kilometres), colouring each point by the grayscale
    PixelValue radiance when available."""
    print(f"  ☁️  Creating pointcloud...")

    coord_cols = [col for col in df.columns if 'BodyFixedCoordinate' in col]
    if len(coord_cols) < 3:
        print(f"     ⚠ BodyFixedCoordinate columns not found")
        print(f"     Available columns: {list(df.columns)[:10]}...")
        print(f"     Skipping pointcloud generation")
        return

    if "BodyFixedCoordinateX" in df.columns:
        x = df["BodyFixedCoordinateX"].to_numpy(dtype=np.float64)
        y = df["BodyFixedCoordinateY"].to_numpy(dtype=np.float64)
        z = df["BodyFixedCoordinateZ"].to_numpy(dtype=np.float64)
    elif "BodyFixedCoordinate" in df.columns:
        # Single packed column — format-dependent parsing would be needed.
        print(f"     ⚠ Single BodyFixedCoordinate column found — needs "
              f"custom parsing")
        return
    else:
        print(f"     ⚠ No BodyFixedCoordinate columns found")
        return

    valid_mask = ~(np.isnan(x) | np.isnan(y) | np.isnan(z))

    # Map PixelValue to grayscale 0-255 where available.
    if "PixelValue" in df.columns:
        pixel_values = df["PixelValue"].to_numpy(dtype=np.float64)
        valid_mask = valid_mask & ~np.isnan(pixel_values)

        pv_valid = pixel_values[valid_mask]
        if len(pv_valid) > 0:
            pv_min, pv_max = pv_valid.min(), pv_valid.max()
            print(f"     PixelValue range: [{pv_min:.2f}, {pv_max:.2f}]")

            # Min-max normalise to 0..255.
            pixel_norm = np.zeros(len(pixel_values), dtype=np.float64)
            pixel_norm[valid_mask] = (pixel_values[valid_mask] - pv_min) / \
                                     (pv_max - pv_min)
            colors = (pixel_norm * 255).astype(np.uint8)
        else:
            print(f"     ⚠ No valid PixelValue found, using gray (128)")
            colors = np.full(len(df), 128, dtype=np.uint8)
    else:
        print(f"     ⚠ PixelValue column not found, using gray (128)")
        colors = np.full(len(df), 128, dtype=np.uint8)

    x_valid = x[valid_mask]
    y_valid = y[valid_mask]
    z_valid = z[valid_mask]
    colors_valid = colors[valid_mask]

    print(f"     Valid points: {len(x_valid)} / {len(df)} "
          f"({len(x_valid)/len(df)*100:.1f}%)")
    print(f"     Coordinate range (MOON_ME, km):")
    print(f"       X: [{x_valid.min():.6f}, {x_valid.max():.6f}]")
    print(f"       Y: [{y_valid.min():.6f}, {y_valid.max():.6f}]")
    print(f"       Z: [{z_valid.min():.6f}, {z_valid.max():.6f}]")

    if len(x_valid) == 0:
        print(f"     ⚠ No valid points to save")
        return

    # PLY
    ply_path = f"{output_base}.ply"
    with open(ply_path, 'w') as f:
        f.write("ply\n")
        f.write("format ascii 1.0\n")
        f.write("comment MOON_ME Body-Fixed Coordinate System\n")
        f.write("comment Units: kilometers\n")
        f.write("comment Color: PixelValue normalized to 0-255 grayscale\n")
        f.write(f"element vertex {len(x_valid)}\n")
        f.write("property float x\n")
        f.write("property float y\n")
        f.write("property float z\n")
        f.write("property uchar red\n")
        f.write("property uchar green\n")
        f.write("property uchar blue\n")
        f.write("end_header\n")

        for i in range(len(x_valid)):
            f.write(f"{x_valid[i]:.6f} {y_valid[i]:.6f} {z_valid[i]:.6f} ")
            f.write(f"{colors_valid[i]} {colors_valid[i]} "
                    f"{colors_valid[i]}\n")

    print(f"     ✓ Saved: {ply_path}")

    # XYZ
    xyz_path = f"{output_base}.xyz"
    np.savetxt(
        xyz_path,
        np.column_stack([x_valid, y_valid, z_valid]),
        fmt='%.6f', delimiter=' ',
        header='X Y Z (MOON_ME, km)', comments='# ',
    )
    print(f"     ✓ Saved: {xyz_path}")

    # XYZRGB
    xyzrgb_path = f"{output_base}.xyzrgb"
    np.savetxt(
        xyzrgb_path,
        np.column_stack([x_valid, y_valid, z_valid,
                         colors_valid, colors_valid, colors_valid]),
        fmt='%.6f %.6f %.6f %d %d %d', delimiter=' ',
        header='X Y Z R G B (MOON_ME km, grayscale 0-255)', comments='# ',
    )
    print(f"     ✓ Saved: {xyzrgb_path}")


# ---------------------------------------------------------------------------
# Top-level driver functions
# ---------------------------------------------------------------------------

def process_single_file(csv_path: str, output_dir: str,
                        product_id: str = None,
                        create_images: bool = True,
                        create_pc: bool = True):
    """Run the full CSV -> parquet + PNGs + pointcloud pipeline on one file."""
    print(f"\n{'='*60}")
    print(f"Processing: {csv_path}")
    print(f"{'='*60}")

    if product_id is None:
        product_id = parse_product_id(csv_path)

    if product_id is None:
        print("  ⚠ Product ID not found in filename and not provided")
        product_id_str = "unknown"
    else:
        product_id_str = product_id
        print(f"  📋 Product ID: {product_id_str}")

    os.makedirs(output_dir, exist_ok=True)

    base_name = Path(csv_path).stem
    output_base = os.path.join(output_dir, base_name)

    df = load_csv_to_dataframe(csv_path)

    has_lookdir = all(c in df.columns for c in
                      ["LookDirectionCameraX",
                       "LookDirectionCameraY",
                       "LookDirectionCameraZ"])
    has_slant = "SlantDistance" in df.columns

    if has_lookdir and has_slant:
        df = add_zdepth_column(df, product_id)
    else:
        print("  ⚠ Skipping Z-Depth calculation (missing required columns)")
        if not has_lookdir:
            print("     Missing: LookDirectionCameraX/Y/Z")
        if not has_slant:
            print("     Missing: SlantDistance")

    # Parquet
    parquet_path = f"{output_base}_zdepth.parquet"
    print(f"  💾 Saving parquet...")
    df.to_parquet(parquet_path, index=False, compression='snappy')
    print(f"     ✓ Saved: {parquet_path}")

    # Depth map images
    if create_images:
        print(f"  🖼️  Creating depth map images...")

        if "PixelValue" in df.columns:
            create_depth_map_image(
                df, "PixelValue",
                f"{output_base}.pixelvalue.png",
                mode='L',
            )

        if "SlantDistance" in df.columns:
            create_depth_map_image(
                df, "SlantDistance",
                f"{output_base}.slantdistance.png",
                cmap='turbo',
            )
            create_depth_map_plot(
                df, "SlantDistance",
                f"{output_base}.slantdistance_plot.png",
                f"Slant Distance - {product_id_str}",
                "km",
            )

        if "ZDepth" in df.columns:
            create_depth_map_image(
                df, "ZDepth",
                f"{output_base}.zdepth.png",
                cmap='turbo',
            )
            create_depth_map_plot(
                df, "ZDepth",
                f"{output_base}.zdepth_plot.png",
                f"Z-Depth - {product_id_str}",
                "km",
            )

    # Pointcloud
    if create_pc:
        create_pointcloud(df, f"{output_base}_pointcloud", product_id_str)

    print(f"\n✅ Processing complete: {base_name}")
    print(f"{'='*60}\n")


def process_directory(directory: str, output_dir: str = None,
                      create_images: bool = True, create_pc: bool = True):
    """Process every *.csv in ``directory`` sequentially."""
    csv_files = sorted(glob.glob(os.path.join(directory, "*.csv")))

    if not csv_files:
        print(f"No CSV files found in {directory}")
        return

    if output_dir is None:
        output_dir = os.path.join(directory, "output")

    print(f"\n{'#'*60}")
    print(f"# Lunar Studio — csv2parquet.py")
    print(f"# Found {len(csv_files)} CSV files")
    print(f"# Output directory: {output_dir}")
    print(f"{'#'*60}")

    success_count = 0
    fail_count = 0

    for csv_file in csv_files:
        try:
            process_single_file(
                csv_file, output_dir,
                create_images=create_images,
                create_pc=create_pc,
            )
            success_count += 1
        except Exception as e:
            print(f"  ❌ Error processing {csv_file}: {e}")
            import traceback
            traceback.print_exc()
            fail_count += 1

    print(f"\n{'#'*60}")
    print(f"# Pipeline complete")
    print(f"# Success: {success_count} / Failed: {fail_count}")
    print(f"{'#'*60}\n")


def process_regions(region_dirs: list, output_base_dir: str = None,
                    create_images: bool = True, create_pc: bool = True):
    """Recurse across region directories and process every site's CSVs.

    Args:
        region_dirs: list of region directory names (e.g. ['nacdtm', 'sldem']).
        output_base_dir: base output directory; if None, each site's output
            lands in ``<site>/output/``.
        create_images: generate depth-map PNGs.
        create_pc: generate pointcloud files.

    Directory layout expected::

        region/
        ├── location1/
        │   ├── site1/
        │   │   ├── M*.echo.*.csv     <- processed
        │   │   └── output/            <- results land here
        │   └── site2/
        └── location2/
            └── site1/

    Only CSVs whose filename starts with ``M`` and contains ``.echo.`` are
    processed (this filters out stray intermediate files).
    """
    print(f"\n{'#'*70}")
    print(f"# Lunar Studio — regional csv2parquet")
    print(f"# Regions: {', '.join(region_dirs)}")
    print(f"{'#'*70}\n")

    total_success = 0
    total_fail = 0
    total_skip = 0

    for region_dir in region_dirs:
        if not os.path.exists(region_dir):
            print(f"⚠ Region directory not found: {region_dir}")
            continue

        print(f"\n{'='*70}")
        print(f"Processing Region: {region_dir}")
        print(f"{'='*70}")

        location_dirs = [d for d in glob.glob(os.path.join(region_dir, "*"))
                         if os.path.isdir(d)]
        if not location_dirs:
            print(f"  No location directories found in {region_dir}")
            continue

        print(f"  Found {len(location_dirs)} location(s)")

        for location_dir in sorted(location_dirs):
            location_name = os.path.basename(location_dir)

            site_pattern = os.path.join(location_dir, "site*")
            site_dirs = [d for d in glob.glob(site_pattern)
                         if os.path.isdir(d)]

            # If there are no site<N>/ sub-dirs, treat the location itself
            # as the site.
            if not site_dirs:
                site_dirs = [location_dir]

            for site_dir in sorted(site_dirs):
                site_name = os.path.basename(site_dir)

                all_csv_files = glob.glob(os.path.join(site_dir, "*.csv"))

                # Only keep M*.echo.*.csv — i.e. per-image campt outputs.
                csv_files = []
                for csv_file in all_csv_files:
                    filename = os.path.basename(csv_file)
                    if filename.startswith('M') and '.echo.' in filename:
                        csv_files.append(csv_file)

                csv_files = sorted(csv_files)

                if not csv_files:
                    print(f"  ⏭️  Skipping {location_name}/{site_name}: "
                          f"no NAC image CSV files (M*.echo.*.csv)")
                    total_skip += 1
                    continue

                print(f"\n  📍 Location: {location_name} / Site: {site_name}")
                print(f"     Found {len(csv_files)} NAC image CSV file(s)")

                if output_base_dir:
                    output_dir = os.path.join(
                        output_base_dir,
                        os.path.basename(region_dir),
                        location_name,
                        site_name,
                    )
                else:
                    output_dir = os.path.join(site_dir, "output")

                for csv_file in csv_files:
                    try:
                        csv_name = os.path.basename(csv_file)
                        print(f"\n     🔄 Processing: {csv_name}")

                        process_single_file(
                            csv_file, output_dir,
                            create_images=create_images,
                            create_pc=create_pc,
                        )
                        total_success += 1
                    except Exception as e:
                        print(f"     ❌ Error processing {csv_file}: {e}")
                        import traceback
                        traceback.print_exc()
                        total_fail += 1

    print(f"\n{'#'*70}")
    print(f"# Regional processing complete")
    print(f"# Success: {total_success} files")
    print(f"# Failed: {total_fail} files")
    print(f"# Skipped: {total_skip} sites (no NAC image CSV files)")
    print(f"{'#'*70}\n")


# ---------------------------------------------------------------------------
# CLI
# ---------------------------------------------------------------------------

def main():
    parser = argparse.ArgumentParser(
        description=(
            "Lunar Studio csv2parquet — CSV → Z-Depth Parquet → Images → "
            "Pointcloud"
        ),
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog="""
Examples:
  # Auto-detect region directories in the current folder
  python3 csv2parquet.py

  # Process a single file
  python3 csv2parquet.py --input data.csv --product-id M1121224102LE

  # Process every CSV in a directory
  python3 csv2parquet.py --directory ./csv_files --output ./results

  # Recurse across region directories
  python3 csv2parquet.py --regions nacdtm sldem
  python3 csv2parquet.py --regions nacdtm sldem --output ./results

  # Skip pointcloud generation
  python3 csv2parquet.py --no-pointcloud
        """,
    )

    parser.add_argument("--input", "-i", type=str,
                        help="Input CSV file")
    parser.add_argument("--directory", "-d", type=str,
                        help="Directory containing CSV files")
    parser.add_argument("--regions", "-r", type=str, nargs='+',
                        help="Region directories to process (e.g. nacdtm "
                             "sldem). If omitted, known region names are "
                             "auto-detected in the current directory.")
    parser.add_argument("--output", "-o", type=str,
                        help="Output directory "
                             "(default: ./output or <dir>/output or "
                             "<site>/output)")
    parser.add_argument("--product-id", "-p", type=str,
                        help="Product ID (e.g. M1121224102LE)")
    parser.add_argument("--no-images", action="store_true",
                        help="Skip depth-map image generation")
    parser.add_argument("--no-pointcloud", action="store_true",
                        help="Skip pointcloud generation")

    args = parser.parse_args()

    input_count = sum([bool(args.input), bool(args.directory),
                       bool(args.regions)])

    # If nothing is specified, try to auto-detect known region sub-directories
    # in the current working directory.
    if input_count == 0:
        print("No input specified. Auto-detecting regions in current "
              "directory...")

        known_regions = ['nacdtm', 'sldem', 'nac_apollo15', 'luti']
        detected_regions = [r for r in known_regions
                            if os.path.exists(r) and os.path.isdir(r)]

        if detected_regions:
            print(f"Detected regions: {', '.join(detected_regions)}")
            args.regions = detected_regions
        else:
            print("No known region directories found "
                  "(nacdtm, sldem, nac_apollo15, luti).")
            print("Please specify --input, --directory, or --regions.")
            return 1
    elif input_count > 1:
        parser.error("Cannot specify multiple input methods "
                     "(--input, --directory, --regions)")

    if args.output:
        output_dir = args.output
    elif args.input:
        output_dir = os.path.join(os.path.dirname(args.input) or ".",
                                  "output")
    else:
        output_dir = None  # resolved inside process_directory / _regions

    try:
        if args.input:
            if not os.path.exists(args.input):
                print(f"Error: file {args.input} does not exist")
                return 1

            process_single_file(
                args.input, output_dir, args.product_id,
                create_images=not args.no_images,
                create_pc=not args.no_pointcloud,
            )

        elif args.directory:
            if not os.path.exists(args.directory):
                print(f"Error: directory {args.directory} does not exist")
                return 1

            process_directory(
                args.directory, output_dir,
                create_images=not args.no_images,
                create_pc=not args.no_pointcloud,
            )

        elif args.regions:
            process_regions(
                args.regions, output_dir,
                create_images=not args.no_images,
                create_pc=not args.no_pointcloud,
            )

        return 0

    except Exception as e:
        print(f"\n❌ Fatal error: {e}")
        import traceback
        traceback.print_exc()
        return 1


if __name__ == "__main__":
    exit(main())
