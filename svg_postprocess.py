#!/usr/bin/env python3
"""
SVG post-processing script:
- Replace all xlink:href with href
- Renumber link IDs and their references deterministically (link1, link2, ...)
Usage: python3 svg_postprocess.py input.svg [output.svg]
If output.svg is not specified, input.svg is overwritten.
"""
import sys
import re
from pathlib import Path

def normalize_float(match):
    """Convert float numbers in SVG paths to use only one decimal place."""
    num = float(match.group(0))
    return f"{num:.1f}"

def postprocess_svg(input_path, output_path=None):
    with open(input_path, 'r', encoding='utf-8') as f:
        svg = f.read()
    # Replace xlink:href with href
    svg = svg.replace('xlink:href', 'href')

    # Normalize floating point numbers only in path data and specific graphical attributes
    def normalize_path_data(match):
        """Normalize numbers within path data while preserving commands."""
        data = match.group(1)
        # Split into commands and coordinates while preserving the commands
        parts = re.split(r'([A-Za-z])', data)
        normalized_parts = []
        for part in parts:
            if re.match(r'[A-Za-z]', part):  # SVG command
                normalized_parts.append(part)
            else:  # coordinate data
                # Normalize numbers but preserve spacing
                normalized = re.sub(r'-?\d+\.\d+', normalize_float, part)
                normalized_parts.append(normalized)
        return 'd="' + ''.join(normalized_parts) + '"'

    # First handle path data specially
    svg = re.sub(r'd="([^"]*)"', normalize_path_data, svg)

    # Process SVG content in stages to handle different contexts correctly

    # Stage 1: Handle path data carefully
    svg = re.sub(r'd="([^"]*)"', normalize_path_data, svg)

    # Stage 2: Handle specific path-related and graphical attributes
    graphical_attrs = [
        (r'points="([^"]*)"', 'points'),  # polygon/polyline points
        (r'x="([^"]*)"', 'x'),
        (r'y="([^"]*)"', 'y'),
        (r'width="([^"]*)"', 'width'),
        (r'height="([^"]*)"', 'height'),
        (r'cx="([^"]*)"', 'cx'),
        (r'cy="([^"]*)"', 'cy'),
        (r'r="([^"]*)"', 'r')
    ]

    for pattern, attr_name in graphical_attrs:
        def normalize_specific_attr(match, attr=attr_name):
            value = match.group(1)
            # Skip if within a text element context
            prev_content = svg[:match.start()]
            if '<text' in prev_content and '</text' not in prev_content:
                return f'{attr}="{value}"'
            return f'{attr}="{re.sub(r"-?\d+\.\d+", normalize_float, value)}"'

        svg = re.sub(pattern, normalize_specific_attr, svg)

    # Remove trailing whitespace from all lines
    svg = '\n'.join(line.rstrip() for line in svg.splitlines())    # Find and normalize all IDs with random hex suffixes
    # Pattern matches IDs like p117440ae8c, maeebeef912, etc.
    id_patterns = [
        (r'id="([mp][a-f0-9]{9,10})"', lambda m, i: f'{m[0]}{i+1}'),  # p/m + hex
        (r'href="#([mp][a-f0-9]{9,10})"', lambda m, i: f'{m[0]}{i+1}'),  # p/m + hex in hrefs
        (r'clip-path="url\(#([mp][a-f0-9]{9,10})\)"', lambda m, i: f'{m[0]}{i+1}'),  # p/m + hex in clip-paths
        (r'id="(link\d+)"', lambda m, i: f'link{i+1}')  # Original link pattern
    ]

    for pattern, id_format in id_patterns:
        # Find all matches for this pattern
        matches = re.finditer(pattern, svg)
        ids = []
        for match in matches:
            full_id = match.group(1)
            if full_id not in ids:
                ids.append(full_id)

        # Create mapping from old to new IDs
        id_map = {old: id_format(old[0], i) for i, old in enumerate(ids)}

        # Replace all occurrences
        for old, new in id_map.items():
            svg = re.sub(rf'id="{old}"', f'id="{new}"', svg)
            svg = re.sub(rf'href="#{old}"', f'href="#{new}"', svg)
            svg = re.sub(rf'clip-path="url\(#{old}\)"', f'clip-path="url(#{new})"', svg)
    out_path = output_path if output_path else input_path
    with open(out_path, 'w', encoding='utf-8') as f:
        f.write(svg)

if __name__ == "__main__":
    if len(sys.argv) < 2 or len(sys.argv) > 3:
        print("Usage: python3 svg_postprocess.py input.svg [output.svg]")
        sys.exit(1)
    input_svg = sys.argv[1]
    output_svg = sys.argv[2] if len(sys.argv) == 3 else None
    if not Path(input_svg).exists():
        print(f"Error: File {input_svg} not found")
        sys.exit(1)
    postprocess_svg(input_svg, output_svg)
