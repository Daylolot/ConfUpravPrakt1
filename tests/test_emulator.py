"""Integration tests: run the real executable with isolated startup scripts."""
import pathlib
import re
import subprocess
import tempfile
import unittest

ROOT = pathlib.Path(__file__).resolve().parents[1]


class EmulatorTests(unittest.TestCase):
    def run_shell(self, commands, vfs="examples/commands.xml"):
        # Temporary scripts never modify repository fixtures.
        with tempfile.TemporaryDirectory() as folder:
            script = pathlib.Path(folder) / "commands.txt"
            script.write_text(commands + "\nexit\n", encoding="utf-8")
            args = [str(ROOT / "emulator")]
            if vfs is not None:
                args += ["--vfs", vfs]
            args += ["--script", str(script)]
            result = subprocess.run(args, cwd=ROOT, input="", text=True,
                                    capture_output=True, timeout=5)
        # Remove launch settings and echoed commands, retaining command output.
        lines = [line for line in result.stdout.splitlines()
                 if not line.startswith(("vfs_path=", "script_path="))
                 and not re.match(r"^.*@.*:\S*\$ ", line)]
        return result, lines

    def check_output(self, commands, expected, vfs="examples/commands.xml"):
        result, lines = self.run_shell(commands, vfs)
        self.assertEqual(result.returncode, 0, result.stderr)
        self.assertEqual(result.stderr, "")
        self.assertEqual(lines, expected)

    def test_pwd_root(self):
        self.check_output("pwd", ["/"])

    def test_ls_root(self):
        self.check_output("ls", ["readme.txt", "docs"])

    def test_ls_file(self):
        self.check_output("ls /readme.txt", ["readme.txt"])

    def test_cd_absolute(self):
        self.check_output("cd /docs\npwd", ["/docs"])

    def test_cd_relative_parent_and_dot(self):
        self.check_output("cd docs\ncd archive\ncd ..\ncd .\npwd", ["/docs"])

    def test_cd_default_and_root_parent(self):
        self.check_output("cd docs\ncd\ncd ..\npwd", ["/"])

    def test_uniq(self):
        self.check_output("uniq /docs/repeat.txt", ["apple", "banana", "apple"])

    def test_uniq_count(self):
        self.check_output("uniq -c /docs/repeat.txt",
                          ["      2 apple", "      1 banana", "      2 apple"])

    def test_uniq_duplicates(self):
        self.check_output("uniq -d /docs/repeat.txt", ["apple", "apple"])

    def test_uniq_unique(self):
        self.check_output("uniq -u /docs/repeat.txt", ["banana"])

    def test_copy_file(self):
        self.check_output("cp /readme.txt /copy.txt\nuniq /copy.txt", ["Virtual home"])

    def test_copy_into_directory(self):
        self.check_output("cp /readme.txt /docs/archive\nuniq /docs/archive/readme.txt",
                          ["Virtual home"])

    def test_copy_overwrite(self):
        self.check_output("cp /readme.txt /docs/repeat.txt\nuniq /docs/repeat.txt",
                          ["Virtual home"])

    def test_copy_recursive(self):
        self.check_output("cp -r /docs /backup\nuniq /backup/archive/old.txt",
                          ["Old version"])

    def test_copy_is_memory_only(self):
        fixture = ROOT / "examples/commands.xml"
        before = fixture.read_bytes()
        self.check_output("cp /readme.txt /copy.txt\nls /copy.txt", ["copy.txt"])
        self.assertEqual(fixture.read_bytes(), before)
        result, _ = self.run_shell("ls /copy.txt")
        self.assertEqual(result.returncode, 1)
        self.assertIn("no such file or directory", result.stderr)

    def test_command_errors(self):
        cases = [("unknown", "command not found"),
                 ("ls /missing", "no such file or directory"),
                 ("cd /readme.txt", "not a directory"),
                 ("pwd extra", "too many arguments"),
                 ("uniq", "missing file operand"),
                 ("uniq -x /readme.txt", "unsupported option"),
                 ("cp /docs /copy", "use -r"),
                 ("cp /readme.txt /readme.txt", "same file"),
                 ("cp -r /docs /docs/archive", "into itself"),
                 ("cp /missing /copy", "no such file or directory"),
                 ("cp /readme.txt", "missing source or destination")]
        for command, message in cases:
            with self.subTest(command=command):
                result, _ = self.run_shell(command)
                self.assertEqual(result.returncode, 1)
                self.assertIn(message, result.stderr)

    def test_xml_entities(self):
        self.check_output("uniq /readme.txt", ["Hello & welcome!"], "examples/files.xml")

    def test_base64_binary(self):
        result, _ = self.run_shell("uniq /raw.bin", "examples/files.xml")
        self.assertEqual(result.returncode, 1)
        self.assertIn("binary file", result.stderr)

    def test_bad_xml_and_missing_vfs(self):
        for path in ["examples/invalid.xml", "examples/does-not-exist.xml"]:
            with self.subTest(path=path):
                result, _ = self.run_shell("pwd", path)
                self.assertEqual(result.returncode, 1)
                self.assertIn("vfs:", result.stderr)

    def test_missing_loaded_vfs(self):
        result, _ = self.run_shell("pwd", None)
        self.assertEqual(result.returncode, 1)
        self.assertIn("no VFS loaded", result.stderr)

    def test_script_comments_and_line_numbers(self):
        result, lines = self.run_shell("// comment\n\nunknown\npwd")
        self.assertEqual(result.returncode, 1)
        self.assertIn(":3: command failed", result.stderr)
        self.assertEqual(lines, ["/"])

    def test_invalid_cli_options(self):
        result = subprocess.run([str(ROOT / "emulator"), "--unknown"],
                                cwd=ROOT, input="", text=True,
                                capture_output=True, timeout=5)
        self.assertEqual(result.returncode, 2)
        self.assertIn("usage:", result.stderr)


if __name__ == "__main__":
    unittest.main()
