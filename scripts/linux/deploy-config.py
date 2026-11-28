#!/usr/bin/python3 -I
"""Install this checkout's Keyflow config into an existing service and restart it."""

import argparse
from pathlib import Path
import subprocess
import sys


SOURCE = Path(__file__).resolve().parent
REPO_CONFIG = SOURCE.parents[1] / "src/config.json"
INSTALLED = Path("/etc/keyflow/config.json")
BINARY = Path("/usr/local/lib/keyflow/keyflow")


def check(config, installed):
    """Run check-config.py; return its exit code (0 in sync, 1 drift)."""
    return subprocess.run([sys.executable, "-I", str(SOURCE / "check-config.py"),
                           "--reference", str(config), "--installed", str(installed)]).returncode


def deploy(config, installed=INSTALLED, binary=BINARY, dry_run=False, run=subprocess.run):
    if not installed.exists():
        raise FileNotFoundError(f"No installed service config at {installed}; run install.py first")
    # Validate with the binary the service actually runs, not a local build.
    run([str(binary), "--validate", str(config)], check=True)
    status = check(config, installed)
    if status == 0:
        return "unchanged"
    if status != 1:
        raise RuntimeError("Could not compare configs")
    if dry_run:
        return "would deploy"
    print("Release all keys; Keyflow restarts after installing.")
    run(["sudo", "install", "-m", "0644", str(config), str(installed)], check=True)
    run(["sudo", "systemctl", "restart", "keyflow.service"], check=True)
    if check(config, installed) != 0:
        raise RuntimeError(f"{installed} still differs after install")
    return "deployed"


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--config", type=Path, default=REPO_CONFIG)
    parser.add_argument("--dry-run", action="store_true", help="show drift without installing")
    args = parser.parse_args()
    try:
        result = deploy(args.config.resolve(), dry_run=args.dry_run)
    except (OSError, RuntimeError, subprocess.SubprocessError) as error:
        parser.exit(1, f"Deploy failed: {error}\n")
    print({"unchanged": "Nothing to deploy.", "would deploy": "Dry run: nothing installed.",
           "deployed": "Deployed and restarted keyflow.service."}[result])


if __name__ == "__main__":
    main()
