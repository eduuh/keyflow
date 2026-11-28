# Linux boot service (experimental)

Keyflow uses Interception on Windows and evdev/uinput on Linux. This guide covers
an optional Linux systemd installation, not a built-in autostart feature. The
executable still runs in the foreground. Windows installation is unchanged.

**This changes keyboard capture and system configuration.** Use an explicitly
selected keyboard, keep an unaffected mouse/terminal available, and authorize
the installation separately from unit tests. Do not test by typing secrets.
An unselected keyboard keeps its raw layout.

## What the installation does

The files under [`scripts/linux/`](../scripts/linux/) install a root-owned binary,
config and helpers, a dedicated non-login `keyflow` account, scoped udev rules,
and a `keyflow.service`. They do not download a binary, start capture, enable
autostart, stop Kanata or change your desktop bindings.

The service has no capabilities and does not join the `input` group. Its access
is limited by file permissions to the selected keyboard, `/dev/uinput` and its
own virtual output. The device cgroup permits input reads because the output
event number is allocated dynamically; it does not replace those narrow file
permissions. A private `/run/keyflow` directory supplies `XDG_RUNTIME_DIR`.

Startup waits briefly for every selected key to be released. The backend then
rechecks the physical state and virtual output before grabbing the keyboard.
Held keys, missing access, selected-device loss or queue overflow fail closed.
The service does not restart automatically or reconnect a lost device.

## Prerequisites and device selection

Use Linux with systemd, udev, Python 3, `systemd-sysusers`, `setfacl`, and the
system libevdev runtime. For example:

```sh
# Ubuntu 24.04
sudo apt-get install python3 acl libevdev2

# Arch
sudo pacman -S --needed python acl libevdev
```

Use an existing stable `/dev/input/by-id/...-event-kbd` or
`/dev/input/by-path/...-event-kbd` path. Verify which physical keyboard it names:

```sh
ls -l /dev/input/by-id/ /dev/input/by-path/
udevadm info --query=property --name /dev/input/by-path/YOUR-KEYBOARD-event-kbd
```

Do not select a mouse, another remapper's virtual keyboard or a guessed event
number. The installer accepts one explicitly selected keyboard. The foreground
CLI can select multiple keyboards with repeated `--device` arguments.

## Review and install

Run from a source checkout or extracted Linux release archive containing
`scripts/linux/`. Replace the example paths with your actual binary, JSON config
and verified device:

```sh
python3 -I scripts/linux/install.py \
  --binary "$PWD/keyflow" \
  --config "$PWD/config.json" \
  --device /dev/input/by-path/YOUR-KEYBOARD-event-kbd \
  --dry-run
```

The dry-run validates the config and reports the proposed installation without
changing files, services, accounts or device permissions. A source build can use
`$PWD/builds/Linux-Release/release/bin/keyflow` and `$PWD/src/config.json` instead.

For an explicitly authorized fresh installation:

```sh
sudo python3 -I scripts/linux/install.py \
  --binary "$PWD/keyflow" \
  --config "$PWD/config.json" \
  --device /dev/input/by-path/YOUR-KEYBOARD-event-kbd
```

Existing deployment paths are a refusal, not an upgrade request. **Do not rerun
this installer over a working, manually installed service.** Preserve its config,
inspect the differences and arrange a separate migration. The repository files
are reusable; your installed files and private backup are machine-local.
Verify a downloaded archive's checksum and use the isolated `-I` invocation so
caller-controlled sibling modules or `PYTHONPATH` cannot load into a root Python
process. A failed install can leave provisioning files; inspect them before
attempting cleanup or a new installation.

If replacing an existing Kanata user service, add `--kanata-user "$USER"` to the
review and installation commands. This explicitly selects the account whose
`~/.config/systemd/user/kanata.service` and
`~/.config/keyboard/colemak.kbd` are copied into a root-private archive. It does
not stop, remove or disable Kanata. Review custom service paths before migration.

## Guarded switch from Kanata

These commands require an installed archive and a live session for the selected
Kanata user. First arm and verify an independent restoration timer:

```sh
sudo systemd-run --unit=keyflow-trial-rollback --on-active=180s \
  --timer-property=AccuracySec=1s /usr/local/lib/keyflow/rollback
systemctl is-active keyflow-trial-rollback.timer
```

Only after the timer is active, release all keys and switch:

```sh
systemctl --user stop kanata.service
sudo systemctl start keyflow.service
systemctl is-active keyflow.service
systemctl --user is-active kanata.service
```

Keyflow must be active and Kanata inactive. Verify the desktop has the Keyflow
virtual keyboard and no Kanata keyboard before testing a short sample. Do not
enable both remappers on the same physical keyboard. If startup or typing fails,
restore Kanata with the rollback command or let the timer run.

The bundled config keeps Windows physical key identities. In that layout Enter
is RightShift, not Kanata's tap-hold Enter. Check the layout, OS keymap, NumLock,
modifiers and layers before accepting it at the login screen.

Keep one config, `src/config.json`. Don't hand-edit a separate Linux copy, and
don't edit `/etc/keyflow/config.json` directly: a copy drifts silently from the
repository. Put Linux-only differences in the config's `platformOverrides.linux`
object, which applies only on Linux.

### Update an installed config

To deploy `src/config.json` to an installed service, run this as your normal user:

```sh
python3 -I scripts/linux/deploy-config.py --dry-run   # show drift only
python3 -I scripts/linux/deploy-config.py
```

The script validates the config with the installed binary. If the installed copy
already matches, it does nothing. Otherwise it installs the config with `sudo`,
restarts `keyflow.service`, then confirms the installed copy matches. Release all
keys when it says so.

### Check the installed config

To compare `/etc/keyflow/config.json` with `src/config.json`, run this without sudo:

```sh
python3 -I scripts/linux/check-config.py
```

The check exits 0 when the two match. It exits 1 on drift and prints each differing
key with both values, for example
`remapping.Enter: reference="RightShift" installed="Enter"`. Layer keys are named
by layer. Pass `--reference <file>` to compare against another file.

To get this warning automatically after every `git pull` or checkout, install the
hooks once per clone:

```sh
sh scripts/linux/install-hooks.sh
```

This adds `post-merge` and `post-checkout` hooks to the shared hooks directory,
which covers every worktree. They only warn and never block git. The hooks
compare the two files byte for byte and never run code from the checkout, so
checking out an untrusted branch is safe. The installer leaves any existing hook
of those names untouched.

After successful typing, stop the trial timer and recheck both service states.
If the timer already restored Kanata, start a new guarded trial instead of
enabling an inactive Keyflow service:

```sh
sudo systemctl stop keyflow-trial-rollback.timer keyflow-trial-rollback.service
systemctl is-active keyflow.service
systemctl --user is-active kanata.service
sudo systemctl enable keyflow.service
systemctl --user disable kanata.service
```

Keep the archived Kanata binary and required shared permissions until rollback
and reboot behavior have been checked. The installer does not remove packages,
group memberships or shared udev/module settings. A folded Stow directory can
point into tracked dotfiles: never delete its layout file to retire a symlink.

## Operation and rollback

```sh
systemctl --no-pager status keyflow.service
journalctl -u keyflow.service -e
sudo systemctl stop keyflow.service
sudo /usr/local/lib/keyflow/rollback --prepare
sudo /usr/local/lib/keyflow/rollback
```

Stop returns the selected keyboard to its raw layout. Physical Ctrl+Escape also
stops Keyflow before remapping; it does not automatically start Kanata.
`--prepare` restores the archived configuration without changing either
remapper. Default rollback prepares the archive first, then stops/disables
Keyflow and restores the selected Kanata user service. Without an archive,
rollback stops/disables Keyflow and leaves raw input.

Restoration refuses to overwrite differing files or write through symlinks.
Already identical regular files are left unchanged, including a layout reached
through an existing Stow directory. If the archive or user session
is unavailable, it must not silently stop a working Keyflow in order to promise
a Kanata recovery it cannot perform. Keep an unaffected control channel.
Neither default operation nor the service logs per-key events; leave
`--verbose` off.

Finally, verify a **user-approved reboot**: Keyflow starts before login, the
selected keyboard and mouse work, the original layout is usable, and Kanata
stays disabled. Enabling a unit is not proof of reboot or login-screen behavior.
Hosted CI tests installation logic without changing services or reading devices.
It does not prove a fresh live installation, desktop acceptance or reboot.
