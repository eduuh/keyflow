import argparse
import fcntl
import json
import os
from pathlib import Path
import pwd
import re
import stat
import subprocess


LIBRARY = Path("/usr/local/lib/keyflow")
CONFIG = Path("/etc/keyflow")
OLD_RULE = Path("/etc/udev/rules.d/99-kanata-uinput.rules")
RESTORE_FILE = """
import os,pathlib,stat,sys
path=pathlib.Path(sys.argv[1]); data=sys.stdin.buffer.read()
if path.is_symlink():
    raise SystemExit("Refusing a symlink destination")
try:
    fd=os.open(path,os.O_RDONLY|os.O_NONBLOCK|os.O_NOFOLLOW)
except FileNotFoundError:
    fd=None
if fd is not None:
    with os.fdopen(fd,"rb") as stream:
        if not stat.S_ISREG(os.fstat(stream.fileno()).st_mode) or stream.read()!=data:
            raise SystemExit("Refusing to overwrite changed configuration")
else:
    if not path.is_absolute() or ".." in path.parts:
        raise SystemExit("Unsafe destination")
    parent=os.open("/",os.O_RDONLY|os.O_DIRECTORY|os.O_NOFOLLOW)
    try:
        for part in path.parts[1:-1]:
            try:
                child=os.open(part,os.O_RDONLY|os.O_DIRECTORY|os.O_NOFOLLOW,dir_fd=parent)
            except FileNotFoundError:
                os.mkdir(part,0o700,dir_fd=parent)
                child=os.open(part,os.O_RDONLY|os.O_DIRECTORY|os.O_NOFOLLOW,dir_fd=parent)
            os.close(parent); parent=child
        fd=os.open(path.name,os.O_WRONLY|os.O_CREAT|os.O_EXCL|os.O_NOFOLLOW,0o600,dir_fd=parent)
        os.fchmod(fd,0o600)
        with os.fdopen(fd,"wb") as stream:
            stream.write(data)
    finally:
        os.close(parent)
"""


def command(*arguments, **options):
    options.setdefault("timeout", 30)
    return subprocess.run(arguments, check=True, **options)


def private_bytes(path):
    descriptor = os.open(path, os.O_RDONLY | os.O_NOFOLLOW | os.O_CLOEXEC)
    with os.fdopen(descriptor, "rb") as stream:
        status = os.fstat(stream.fileno())
        if (not stat.S_ISREG(status.st_mode) or status.st_uid != 0
                or stat.S_IMODE(status.st_mode) != 0o600):
            raise ValueError(f"Unsafe root-private archive or metadata: {path}")
        return stream.read()


def user_service(owner, *arguments):
    return command(
        "runuser", "-u", owner.pw_name, "--", "env",
        f"XDG_RUNTIME_DIR=/run/user/{owner.pw_uid}",
        f"DBUS_SESSION_BUS_ADDRESS=unix:path=/run/user/{owner.pw_uid}/bus",
        "systemctl", "--user", *arguments,
    )


def restore_archive(metadata, require_session=True):
    name = metadata.get("user", "")
    if not re.fullmatch(r"[a-z_][a-z0-9_-]*", name):
        raise ValueError("Invalid archived user")
    owner = pwd.getpwnam(name)
    if (owner.pw_uid == 0 or owner.pw_uid != metadata.get("uid")
            or owner.pw_dir != metadata.get("home")):
        raise ValueError("Archived account identity changed")
    home = Path(owner.pw_dir)
    for name, destination in (
        ("colemak.kbd", home / ".config/keyboard/colemak.kbd"),
        ("kanata.service", home / ".config/systemd/user/kanata.service"),
    ):
        command("runuser", "-u", owner.pw_name, "--", "/usr/bin/python3", "-I", "-c",
                RESTORE_FILE, str(destination), input=private_bytes(LIBRARY / "kanata-backup" / name))
    old = LIBRARY / "kanata-backup/99-kanata-uinput.rules"
    if old.exists():
        data = private_bytes(old)
        if OLD_RULE.exists() or OLD_RULE.is_symlink():
            status = OLD_RULE.lstat()
            if (not stat.S_ISREG(status.st_mode) or status.st_uid != 0
                    or status.st_mode & 0o022 or OLD_RULE.read_bytes() != data):
                raise ValueError("Refusing to overwrite a changed legacy udev rule")
        else:
            descriptor = os.open(OLD_RULE, os.O_WRONLY | os.O_CREAT | os.O_EXCL | os.O_NOFOLLOW, 0o644)
            with os.fdopen(descriptor, "wb") as stream:
                os.fchmod(stream.fileno(), 0o644)
                stream.write(data)
        command("udevadm", "control", "--reload")
        command("udevadm", "trigger", "--action=change", "/sys/class/misc/uinput")
        command("udevadm", "settle", "--timeout=10")
    if require_session:
        bus = Path(f"/run/user/{owner.pw_uid}/bus")
        if not bus.is_socket() or bus.stat().st_uid != owner.pw_uid:
            raise ValueError("Kanata user's session bus is unavailable; Keyflow is unchanged")
        user_service(owner, "daemon-reload")
    return owner


def perform_rollback(metadata, prepare=False):
    owner = (restore_archive(metadata["kanata"], require_session=not prepare)
             if metadata.get("kanata") else None)
    if prepare:
        print("Archive prepared; neither remapper changed.")
        return
    command("systemctl", "disable", "--now", "keyflow.service")
    if owner is not None:
        user_service(owner, "enable", "--now", "kanata.service")
        user_service(owner, "is-active", "--quiet", "kanata.service")
        print("Keyflow disabled/stopped; archived Kanata is enabled and active.")
    else:
        print("Keyflow disabled/stopped; no Kanata archive, so input is raw.")


def main():
    parser = argparse.ArgumentParser(description="Restore archived Kanata or return to raw input")
    parser.add_argument("--prepare", action="store_true")
    args = parser.parse_args()
    if os.geteuid() != 0:
        parser.exit(1, "Rollback requires sudo.\n")
    try:
        descriptor = os.open("/run/keyflow-deployment.lock",
                             os.O_RDWR | os.O_CREAT | os.O_NOFOLLOW | os.O_CLOEXEC, 0o600)
        with os.fdopen(descriptor, "rb") as lock:
            status = os.fstat(lock.fileno())
            if status.st_uid != 0 or stat.S_IMODE(status.st_mode) != 0o600:
                raise ValueError("Unsafe deployment lock")
            fcntl.flock(lock, fcntl.LOCK_EX)
            metadata = json.loads(private_bytes(CONFIG / "deployment.json"))
            perform_rollback(metadata, args.prepare)
    except (OSError, ValueError, KeyError, subprocess.SubprocessError) as error:
        parser.exit(1, f"Rollback refused/failed: {error}\n")


if __name__ == "__main__":
    main()
