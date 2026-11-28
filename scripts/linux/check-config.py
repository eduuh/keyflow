#!/usr/bin/python3 -I
"""Report how the installed Keyflow config differs from a reference config."""

import argparse
import json
from pathlib import Path


INSTALLED = Path("/etc/keyflow/config.json")
REPO = Path(__file__).resolve().parents[2] / "src/config.json"
MISSING = object()


def flatten(value, prefix=""):
    """Map dotted paths to leaf values. Named list items are keyed by name."""
    if isinstance(value, dict):
        items = value.items()
    elif isinstance(value, list):
        items = ((item.get("name") if isinstance(item, dict) and "name" in item else str(index),
                  item) for index, item in enumerate(value))
    else:
        return {prefix: value}
    flat = {}
    for key, item in items:
        if str(key).startswith("_"):
            continue
        flat.update(flatten(item, f"{prefix}.{key}" if prefix else str(key)))
    return flat


def differences(reference, installed):
    expected, actual = flatten(reference), flatten(installed)
    return [(path, expected.get(path, MISSING), actual.get(path, MISSING))
            for path in sorted(expected.keys() | actual.keys())
            if expected.get(path, MISSING) != actual.get(path, MISSING)]


def show(value):
    return "(missing)" if value is MISSING else json.dumps(value)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--reference", type=Path, default=REPO)
    parser.add_argument("--installed", type=Path, default=INSTALLED)
    args = parser.parse_args()
    try:
        diff = differences(json.loads(args.reference.read_text()),
                           json.loads(args.installed.read_text()))
    except (OSError, ValueError) as error:
        parser.exit(2, f"Cannot compare configs: {error}\n")
    if not diff:
        print(f"In sync: {args.installed} matches {args.reference}")
        return
    print(f"Drift: {args.installed} differs from {args.reference}")
    for path, expected, actual in diff:
        print(f"  {path}: reference={show(expected)} installed={show(actual)}")
    parser.exit(1)


if __name__ == "__main__":
    main()
