import argparse
import fcntl
import os
import time


def wait_for_release(device, timeout=15, stable=0.3):
    descriptor = os.open(device, os.O_RDONLY | os.O_NONBLOCK | os.O_CLOEXEC)
    deadline = time.monotonic() + timeout
    released_since = None
    try:
        while time.monotonic() < deadline:
            state = bytearray(96)
            request = (2 << 30) | (len(state) << 16) | (ord("E") << 8) | 0x18
            fcntl.ioctl(descriptor, request, state)
            now = time.monotonic()
            if any(state):
                released_since = None
            elif released_since is None:
                released_since = now
            elif now - released_since >= stable:
                return
            time.sleep(0.02)
        raise RuntimeError("Keyboard stayed held; refusing capture")
    finally:
        os.close(descriptor)


if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument("--device", required=True)
    args = parser.parse_args()
    wait_for_release(args.device)
