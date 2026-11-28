import contextlib
import importlib.util
import io
import json
import os
from pathlib import Path
import subprocess
import sys
import tempfile
import types
import unittest
from unittest.mock import patch


ROOT = Path(__file__).resolve().parents[2]


def load(name, file):
    spec = importlib.util.spec_from_file_location(name, ROOT / "scripts/linux" / file)
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module


installer = load("installer", "install.py")
rollback = load("rollback", "rollback.py")
waiter = load("waiter", "wait-all-up.py")
checker = load("checker", "check-config.py")
deployer = load("deployer", "deploy-config.py")


class DeploymentTests(unittest.TestCase):
    def test_isolated_installer_ignores_caller_modules(self):
        with tempfile.TemporaryDirectory() as directory:
            path = Path(directory)
            (path / "subprocess.py").write_text("raise RuntimeError('caller module loaded')")
            script = path / "install.py"
            script.write_bytes((ROOT / "scripts/linux/install.py").read_bytes())
            result = subprocess.run(
                [sys.executable, "-I", str(script), "--help"],
                env={**os.environ, "PYTHONPATH": str(path)}, cwd=path,
                stdout=subprocess.PIPE, stderr=subprocess.PIPE,
            )
            self.assertEqual(result.returncode, 0, result.stderr)

    def test_restore_cannot_create_through_symlinked_parent(self):
        with tempfile.TemporaryDirectory() as directory:
            path = Path(directory)
            outside = path / "outside"
            outside.mkdir()
            link = path / "keyboard"
            link.symlink_to(outside, target_is_directory=True)
            arguments = [sys.executable, "-I", "-c", rollback.RESTORE_FILE, str(link / "config")]
            result = subprocess.run(arguments, input=b"archive", stderr=subprocess.PIPE)
            self.assertNotEqual(result.returncode, 0)
            self.assertFalse((outside / "config").exists())
            (outside / "config").write_bytes(b"archive")
            subprocess.run(arguments, input=b"archive", check=True)
            self.assertEqual((outside / "config").read_bytes(), b"archive")

    def test_existing_service_account_must_not_have_input_group_access(self):
        user = types.SimpleNamespace(pw_uid=65001, pw_gid=65001, pw_shell="/usr/sbin/nologin")
        with patch.object(installer.pwd, "getpwnam", return_value=user), \
                patch.object(installer.grp, "getgrnam", return_value=types.SimpleNamespace(gr_gid=65001)), \
                patch.object(installer.os, "getgrouplist", return_value=[65001, 123]), \
                self.assertRaises(ValueError):
            installer.check_service_account()

    def test_installation_leaves_capture_disabled_and_paths_traversable(self):
        with tempfile.TemporaryDirectory() as directory:
            base = Path(directory)
            plan = {
                "device": "/dev/input/by-id/keyboard-event-kbd", "node": "event9",
                "binary": b"\x7fELF", "config": b"{}", "service": "unit", "rules": "rule",
                "kanata": None, "backup": {},
                "helpers": {"rollback.py": b"helper", "wait-all-up.py": b"helper"},
            }
            replacements = {
                "LIBRARY": base / "lib", "CONFIG": base / "config", "UNIT": base / "unit",
                "RULES": base / "rule", "SYSUSERS": base / "sysuser", "MODULES": base / "module",
            }
            old_mask = os.umask(0o077)
            try:
                with patch.multiple(installer, **replacements), \
                        patch.object(installer.os, "geteuid", return_value=0), \
                        patch.object(installer, "check_targets"), \
                        patch.object(installer, "check_service_account"), \
                        patch.object(installer, "command") as command, \
                        contextlib.redirect_stdout(io.StringIO()):
                    installer.install(plan)
                self.assertEqual((base / "lib").stat().st_mode & 0o777, 0o755)
                self.assertEqual((base / "config").stat().st_mode & 0o777, 0o755)
                self.assertEqual((base / "config/deployment.json").stat().st_mode & 0o777, 0o600)
                operations = [call.args for call in command.call_args_list]
                self.assertNotIn(("systemctl", "start", "keyflow.service"), operations)
                self.assertNotIn(("systemctl", "enable", "keyflow.service"), operations)
                self.assertEqual((base / "config/config.json").read_bytes(), b"{}")
            finally:
                os.umask(old_mask)

    def test_device_interpolation_and_unstable_paths_are_refused(self):
        for device in (
            "/dev/input/event3", "/dev/input/by-id/a%u", "/dev/input/by-id/a*",
            "/dev/input/by-id/a\nExecStart=bad", '/dev/input/by-id/a"bad',
            "/dev/input/by-path/../../etc/shadow",
        ):
            with self.subTest(device=device), self.assertRaises(ValueError):
                installer.validate_device(device)
        self.assertEqual(installer.validate_device("/dev/input/by-path/keyboard-event-kbd"),
                         "/dev/input/by-path/keyboard-event-kbd")

    def test_existing_file_or_dangling_symlink_prevents_installation(self):
        with tempfile.TemporaryDirectory() as directory:
            target = Path(directory) / "deployment"
            target.symlink_to(Path(directory) / "missing")
            with self.assertRaises(FileExistsError):
                installer.check_targets([target])
            target.unlink()
            target.write_text("existing")
            with self.assertRaises(FileExistsError):
                installer.check_targets([target])
            self.assertEqual(target.read_text(), "existing")

    def test_dry_run_never_writes_or_runs_install_commands(self):
        plan = {"device": "/dev/input/by-id/keyboard-event-kbd", "kanata": None}
        with patch.object(installer, "command") as command, \
                patch.object(installer, "write_file") as write, \
                contextlib.redirect_stdout(io.StringIO()):
            installer.install(plan, dry_run=True)
        command.assert_not_called()
        write.assert_not_called()

    def test_root_without_invoking_nonroot_user_cannot_validate_source(self):
        with patch.object(installer.os, "geteuid", return_value=0), \
                patch.dict(os.environ, {"SUDO_UID": "0"}), \
                self.assertRaises(ValueError):
            installer.invoking_user()

    def test_source_validation_and_reads_drop_root_credentials(self):
        user = types.SimpleNamespace(pw_name="tester")
        with patch.object(installer, "command", return_value=types.SimpleNamespace(stdout=b"safe")) as command:
            self.assertEqual(installer.read_input("/source/config.json", user), b"safe")
            self.assertEqual(command.call_args.args[:4], ("runuser", "-u", "tester", "--"))
            installer.validate_config(Path("/source/keyflow"), Path("/source/config.json"), user)
            self.assertEqual(command.call_args.args,
                             ("runuser", "-u", "tester", "--", "/source/keyflow", "--validate", "/source/config.json"))

    def test_rules_grant_only_selected_source_and_owned_output(self):
        device = "/dev/input/by-id/keyboard-event-kbd"
        rules = installer.render_rules(device)
        self.assertIn('SYMLINK=="input/by-id/keyboard-event-kbd"', rules)
        self.assertIn('TAG+="systemd"', rules)
        self.assertIn('ATTRS{name}=="Keyflow virtual keyboard"', rules)
        self.assertIn('ATTRS{phys}=="keyflow/uinput"', rules)
        self.assertNotIn('GROUP="input"', rules)
        self.assertNotIn('0666', rules)

    def test_prepare_never_changes_remappers(self):
        with patch.object(rollback, "restore_archive") as restore, \
                patch.object(rollback, "command") as command, \
                contextlib.redirect_stdout(io.StringIO()):
            rollback.perform_rollback({"kanata": {"user": "tester"}}, prepare=True)
        restore.assert_called_once()
        command.assert_not_called()

    def test_failed_restore_cannot_stop_working_keyflow(self):
        with patch.object(rollback, "restore_archive", side_effect=ValueError("No session bus")), \
                patch.object(rollback, "command") as command, \
                self.assertRaises(ValueError):
            rollback.perform_rollback({"kanata": {"user": "tester"}})
        command.assert_not_called()

    def test_no_archive_rollback_only_disables_keyflow(self):
        with patch.object(rollback, "command") as command, \
                patch.object(rollback, "user_service") as user, \
                contextlib.redirect_stdout(io.StringIO()):
            rollback.perform_rollback({"kanata": None})
        command.assert_called_once_with("systemctl", "disable", "--now", "keyflow.service")
        user.assert_not_called()

    def test_restore_script_refuses_changed_files_and_symlinks(self):
        with tempfile.TemporaryDirectory() as directory:
            target = Path(directory) / "config"
            arguments = ["/usr/bin/python3", "-I", "-c", rollback.RESTORE_FILE, str(target)]
            subprocess.run(arguments, input=b"original", check=True)
            result = subprocess.run(arguments, input=b"changed", stderr=subprocess.PIPE)
            self.assertNotEqual(result.returncode, 0)
            self.assertEqual(target.read_bytes(), b"original")
            link = Path(directory) / "link"
            link.symlink_to(target)
            result = subprocess.run(arguments[:-1] + [str(link)], input=b"original", stderr=subprocess.PIPE)
            self.assertNotEqual(result.returncode, 0)
            self.assertEqual(target.read_bytes(), b"original")

    def test_held_keyboard_times_out_without_logging_key_values(self):
        def held(descriptor, request, state):
            state[0] = 1
        with patch.object(waiter.os, "open", return_value=9), \
                patch.object(waiter.os, "close") as close, \
                patch.object(waiter.fcntl, "ioctl", side_effect=held), \
                self.assertRaisesRegex(RuntimeError, "Keyboard stayed held"):
            waiter.wait_for_release("/fake/device", timeout=0.03)
        close.assert_called_once_with(9)

    def test_startup_requires_continuously_released_keys(self):
        state = {"time": 0.0, "reads": 0}
        def read(descriptor, request, keys):
            keys[0] = int(state["reads"] in (0, 2))
            state["reads"] += 1
        def advance(seconds):
            state["time"] += 0.1
        with patch.object(waiter.os, "open", return_value=9), \
                patch.object(waiter.os, "close") as close, \
                patch.object(waiter.fcntl, "ioctl", side_effect=read), \
                patch.object(waiter.time, "monotonic", side_effect=lambda: state["time"]), \
                patch.object(waiter.time, "sleep", side_effect=advance):
            waiter.wait_for_release("/fake/device", timeout=1)
        self.assertGreaterEqual(state["reads"], 7)
        close.assert_called_once_with(9)


class ConfigDriftTests(unittest.TestCase):
    REFERENCE = {
        "remapping": {"_comment": "x", "Enter": "RightShift"},
        "layers": [{"name": "Numpad Layer", "mappings": {"Enter": "RightShift"}}],
    }

    def run_check(self, installed):
        with tempfile.TemporaryDirectory() as directory:
            reference_path = Path(directory) / "reference.json"
            installed_path = Path(directory) / "installed.json"
            reference_path.write_text(json.dumps(self.REFERENCE))
            installed_path.write_text(json.dumps(installed))
            return subprocess.run(
                [sys.executable, "-I", str(ROOT / "scripts/linux/check-config.py"),
                 "--reference", str(reference_path), "--installed", str(installed_path)],
                stdout=subprocess.PIPE, text=True,
            )

    def test_matching_configs_are_in_sync(self):
        result = self.run_check(self.REFERENCE)
        self.assertEqual(result.returncode, 0)
        self.assertIn("In sync", result.stdout)

    def test_drift_names_each_changed_key_by_layer_name(self):
        installed = {
            "remapping": {"Enter": "Enter", "Tab": "Tab"},
            "layers": [{"name": "Numpad Layer", "mappings": {"Enter": "Enter"}}],
        }
        result = self.run_check(installed)
        self.assertEqual(result.returncode, 1)
        self.assertIn('remapping.Enter: reference="RightShift" installed="Enter"', result.stdout)
        self.assertIn('layers.Numpad Layer.mappings.Enter: reference="RightShift" '
                      'installed="Enter"', result.stdout)
        self.assertIn('remapping.Tab: reference=(missing) installed="Tab"', result.stdout)
        self.assertNotIn("_comment", result.stdout)


class DeployConfigTests(unittest.TestCase):
    def setUp(self):
        self.directory = tempfile.TemporaryDirectory()
        base = Path(self.directory.name)
        self.config = base / "config.json"
        self.installed = base / "installed.json"
        self.config.write_text('{"remapping": {"Enter": "RightShift"}}')
        self.installed.write_text('{"remapping": {"Enter": "Enter"}}')
        self.calls = []

    def tearDown(self):
        self.directory.cleanup()

    def fake_run(self, arguments, check):
        self.calls.append(arguments[:3])
        if arguments[:2] == ["sudo", "install"]:
            Path(arguments[-1]).write_bytes(Path(arguments[-2]).read_bytes())

    def deploy(self, **options):
        with contextlib.redirect_stdout(io.StringIO()):
            return deployer.deploy(self.config, self.installed, Path("/bin/keyflow"),
                                   run=self.fake_run, **options)

    def test_drift_installs_restarts_and_rechecks(self):
        self.assertEqual(self.deploy(), "deployed")
        self.assertEqual(self.installed.read_text(), self.config.read_text())
        self.assertEqual(self.calls, [["/bin/keyflow", "--validate", str(self.config)],
                                      ["sudo", "install", "-m"],
                                      ["sudo", "systemctl", "restart"]])

    def test_in_sync_and_dry_run_never_touch_the_service(self):
        self.assertEqual(self.deploy(dry_run=True), "would deploy")
        self.installed.write_text(self.config.read_text())
        self.assertEqual(self.deploy(), "unchanged")
        self.assertFalse([call for call in self.calls if call[0] == "sudo"])

    def test_refuses_without_an_installed_service(self):
        self.installed.unlink()
        with self.assertRaises(FileNotFoundError):
            self.deploy()
        self.assertEqual(self.calls, [])


if __name__ == "__main__":
    unittest.main()
