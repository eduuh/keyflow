#!/usr/bin/python3 -I

import argparse
import grp
import json
import os
from pathlib import Path
import pwd
import re
import shutil
import stat
import subprocess


LIBRARY = Path("/usr/local/lib/keyflow")
CONFIG = Path("/etc/keyflow")
UNIT = Path("/etc/systemd/system/keyflow.service")
RULES = Path("/etc/udev/rules.d/99-keyflow.rules")
SYSUSERS = Path("/etc/sysusers.d/keyflow.conf")
MODULES = Path("/etc/modules-load.d/keyflow.conf")
TARGETS = (LIBRARY, CONFIG, UNIT, RULES, SYSUSERS, MODULES)
SOURCE = Path(__file__).resolve().parent
READ_FILE = "import pathlib,sys; sys.stdout.buffer.write(pathlib.Path(sys.argv[1]).read_bytes())"


def command(*arguments, **options):
    options.setdefault("timeout", 30)
    return subprocess.run(arguments, check=True, **options)


def validate_device(device):
    if not re.fullmatch(r"/dev/input/(by-id|by-path)/[A-Za-z0-9_.:+-]+", device):
        raise ValueError("Use a safe, explicit /dev/input/by-id/ or by-path/ keyboard path")
    return device


def account(name):
    if not re.fullmatch(r"[a-z_][a-z0-9_-]*", name):
        raise ValueError("Invalid user name")
    user = pwd.getpwnam(name)
    if user.pw_uid == 0 or not Path(user.pw_dir).is_absolute():
        raise ValueError("Select a non-root user with an absolute home directory")
    return user


def invoking_user():
    if os.geteuid() != 0:
        return None
    value = os.environ.get("SUDO_UID", "")
    if not value.isdecimal() or int(value) == 0:
        raise ValueError("Run review as a normal user and installation via sudo")
    return pwd.getpwuid(int(value))


def read_input(path, user):
    if user is None:
        return Path(path).read_bytes()
    return command(
        "runuser", "-u", user.pw_name, "--", "/usr/bin/python3", "-I", "-c",
        READ_FILE, str(path), stdout=subprocess.PIPE,
    ).stdout


def validate_config(binary, config, user):
    arguments = [str(binary), "--validate", str(config)]
    if user is not None:
        arguments = ["runuser", "-u", user.pw_name, "--", *arguments]
    command(*arguments)


def check_targets(targets=TARGETS):
    for target in targets:
        if target.exists() or target.is_symlink():
            raise FileExistsError(f"Fresh installations only; refusing existing {target}")
        for parent in target.parents:
            if parent.is_symlink():
                raise ValueError(f"Refusing symlink installation parent {parent}")
            if parent.exists():
                status = parent.stat()
                if status.st_uid != 0 or status.st_mode & 0o022:
                    raise ValueError(f"Unsafe installation parent {parent}")


def check_service_account():
    try:
        user = pwd.getpwnam("keyflow")
    except KeyError:
        return
    group = grp.getgrnam("keyflow")
    if (user.pw_uid == 0 or group.gr_gid == 0 or user.pw_gid != group.gr_gid
            or Path(user.pw_shell).name not in ("nologin", "false")
            or set(os.getgrouplist("keyflow", user.pw_gid)) != {user.pw_gid}):
        raise ValueError("Existing keyflow account is not a restricted service account")


def render_rules(device):
    link = device.removeprefix("/dev/")
    return (
        f'SUBSYSTEM=="input", KERNEL=="event*", SYMLINK=="{link}", '
        'ENV{ID_INPUT_KEYBOARD}=="1", TAG+="systemd", '
        'RUN+="/usr/bin/setfacl -m u:keyflow:r /dev/input/%k"\n'
        'SUBSYSTEM=="misc", KERNEL=="uinput", '
        'RUN+="/usr/bin/setfacl -m u:keyflow:rw /dev/%k"\n'
        'SUBSYSTEM=="input", KERNEL=="event*", ATTRS{name}=="Keyflow virtual keyboard", '
        'ATTRS{phys}=="keyflow/uinput", GROUP="keyflow", MODE="0660"\n'
    )


def make_plan(args):
    device = validate_device(args.device)
    check_targets()
    check_service_account()
    for dependency in ("systemctl", "systemd-sysusers", "systemd-escape", "systemd-analyze",
                       "udevadm", "setfacl", "modprobe", "runuser"):
        if shutil.which(dependency) is None:
            raise ValueError(f"Missing prerequisite: {dependency}")
    user = invoking_user()
    binary = Path(args.binary)
    config = Path(args.config)
    if not binary.is_absolute() or not config.is_absolute():
        raise ValueError("Binary and config paths must be absolute")
    binary_data = read_input(binary, user)
    config_data = read_input(config, user)
    if not binary_data.startswith(b"\x7fELF"):
        raise ValueError("The binary must be a native Linux ELF executable")
    json.loads(config_data)
    validate_config(binary, config, user)
    if not stat.S_ISCHR(os.stat(device).st_mode):
        raise ValueError("Selected input is not a character device")
    properties = command(
        "udevadm", "info", "--query=property", "--name", device,
        stdout=subprocess.PIPE, text=True,
    ).stdout
    properties = dict(line.split("=", 1) for line in properties.splitlines() if "=" in line)
    if properties.get("ID_INPUT_KEYBOARD") != "1":
        raise ValueError("Selected input is not identified as a keyboard")
    node = Path(device).resolve().name
    if not re.fullmatch(r"event[0-9]+", node):
        raise ValueError("Selected keyboard does not resolve to an event node")
    unit_name = command(
        "systemd-escape", "--path", "--suffix=device", device,
        stdout=subprocess.PIPE, text=True,
    ).stdout.strip()
    if not re.fullmatch(r"[A-Za-z0-9_.\\-]+\.device", unit_name):
        raise ValueError("Unexpected escaped device unit")
    template = read_input(SOURCE / "keyflow.service.in", user).decode()
    service = template.replace("@DEVICE_UNIT@", unit_name).replace("@DEVICE@", device)
    plan = {
        "device": device, "node": node, "binary": binary_data, "config": config_data,
        "service": service, "rules": render_rules(device), "kanata": None, "backup": {},
        "helpers": {name: read_input(SOURCE / name, user)
                    for name in ("rollback.py", "wait-all-up.py")},
    }
    if args.kanata_user:
        owner = account(args.kanata_user)
        home = Path(owner.pw_dir)
        # Read user-controlled paths as that user, never with root access.
        reader = owner if os.geteuid() == 0 else None
        if reader is None and os.geteuid() != owner.pw_uid:
            raise ValueError("Review another user's archive via sudo")
        for name, path in (
            ("kanata.service", home / ".config/systemd/user/kanata.service"),
            ("colemak.kbd", home / ".config/keyboard/colemak.kbd"),
        ):
            plan["backup"][name] = read_input(path, reader)
        old_rule = Path("/etc/udev/rules.d/99-kanata-uinput.rules")
        if old_rule.exists():
            status = old_rule.lstat()
            if not stat.S_ISREG(status.st_mode) or status.st_uid != 0 or status.st_mode & 0o022:
                raise ValueError("Unsafe legacy Kanata udev rule")
            plan["backup"][old_rule.name] = old_rule.read_bytes()
        plan["kanata"] = {"user": owner.pw_name, "uid": owner.pw_uid, "home": owner.pw_dir}
    return plan


def make_directory(path):
    if path.is_symlink():
        raise ValueError(f"Refusing symlink directory {path}")
    if path.exists():
        if not path.is_dir():
            raise ValueError(f"Not a directory: {path}")
        return
    make_directory(path.parent)
    path.mkdir(mode=0o755)
    path.chmod(0o755)


def write_file(path, data, mode):
    make_directory(path.parent)
    descriptor = os.open(path, os.O_WRONLY | os.O_CREAT | os.O_EXCL | os.O_NOFOLLOW, mode)
    with os.fdopen(descriptor, "wb") as stream:
        os.fchmod(stream.fileno(), mode)
        stream.write(data if isinstance(data, bytes) else data.encode())


def install(plan, dry_run=False):
    if dry_run:
        print(json.dumps({
            "device": plan["device"], "paths": [str(path) for path in TARGETS],
            "kanata": plan["kanata"], "starts_capture": False,
        }, indent=2))
        return
    if os.geteuid() != 0:
        raise PermissionError("Installation requires sudo; use --dry-run for review")
    check_targets()
    LIBRARY.mkdir(mode=0o755)
    CONFIG.mkdir(mode=0o755)
    LIBRARY.chmod(0o755)
    CONFIG.chmod(0o755)
    write_file(LIBRARY / "keyflow", plan["binary"], 0o755)
    write_file(CONFIG / "config.json", plan["config"], 0o644)
    write_file(CONFIG / "deployment.json", json.dumps({"kanata": plan["kanata"]}), 0o600)
    write_file(SYSUSERS, 'u keyflow - "Keyflow keyboard remapper" /nonexistent /usr/sbin/nologin\n', 0o644)
    command("systemd-sysusers", str(SYSUSERS))
    check_service_account()
    for name in ("rollback.py", "wait-all-up.py"):
        write_file(LIBRARY / name, plan["helpers"][name], 0o644)
    write_file(LIBRARY / "rollback", "#!/bin/sh\nexec /usr/bin/python3 -I /usr/local/lib/keyflow/rollback.py \"$@\"\n", 0o755)
    if plan["backup"]:
        backup = LIBRARY / "kanata-backup"
        backup.mkdir(mode=0o700)
        backup.chmod(0o700)
        for name, data in plan["backup"].items():
            write_file(backup / name, data, 0o600)
    write_file(UNIT, plan["service"], 0o644)
    write_file(RULES, plan["rules"], 0o644)
    write_file(MODULES, "uinput\n", 0o644)
    command("systemd-analyze", "verify", str(UNIT))
    command("runuser", "-u", "keyflow", "--", str(LIBRARY / "keyflow"),
            "--validate", str(CONFIG / "config.json"))
    command("modprobe", "uinput")
    command("udevadm", "control", "--reload")
    command("udevadm", "trigger", "--action=change",
            f"/sys/class/input/{plan['node']}", "/sys/class/misc/uinput")
    command("udevadm", "settle", "--timeout=10")
    command("systemctl", "daemon-reload")
    print("Installed but not started or enabled. Existing remappers are unchanged.")


def main():
    parser = argparse.ArgumentParser(description="Fresh, opt-in Linux Keyflow service installation")
    parser.add_argument("--binary", required=True)
    parser.add_argument("--config", required=True)
    parser.add_argument("--device", required=True)
    parser.add_argument("--kanata-user")
    parser.add_argument("--dry-run", action="store_true")
    args = parser.parse_args()
    try:
        install(make_plan(args), args.dry_run)
    except (OSError, ValueError, KeyError, subprocess.SubprocessError) as error:
        parser.exit(1, f"Installation refused/failed: {error}. No capture was started.\n")


if __name__ == "__main__":
    main()
