#!/usr/bin/env python3
import pathlib
import subprocess
import tempfile
import unittest


ROOT = pathlib.Path(__file__).resolve().parents[1]
BINARY = ROOT / "build" / "hash_generator"


def run(*args, data=None):
    result = subprocess.run([str(BINARY), *args], input=data, capture_output=True)
    return result


class HashGeneratorTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        BINARY.parent.mkdir(exist_ok=True)
        subprocess.run(
            ["c++", "-std=c++20", "-O2", "-Wall", "-Wextra", "-pedantic",
             "src/main.cpp", "-o", str(BINARY)],
            cwd=ROOT,
            check=True,
        )

    def test_empty_and_one_byte_inputs_have_256_bit_hex_output(self):
        for text in ("", "a", "b", "Lietuva!", "ąžuolas"):
            result = run("--text", text)
            self.assertEqual(result.returncode, 0)
            digest = result.stdout.decode().strip()
            self.assertRegex(digest, r"^[0-9a-f]{64}$")

    def test_determinism_and_input_sensitivity(self):
        first = run("--text", "A").stdout
        self.assertEqual(first, run("--text", "A").stdout)
        self.assertNotEqual(first, run("--text", "B").stdout)
        self.assertNotEqual(first, run("--text", "A ").stdout)

    def test_file_and_text_modes_use_the_same_bytes(self):
        with tempfile.NamedTemporaryFile() as file:
            file.write("ąžuolas\n".encode("utf-8"))
            file.flush()
            file_digest = run("--file", file.name).stdout
        text_digest = run("--text", "ąžuolas\n").stdout
        self.assertEqual(file_digest, text_digest)

    def test_file_errors_are_reported(self):
        result = run("--file", str(ROOT / "does-not-exist"))
        self.assertNotEqual(result.returncode, 0)
        self.assertIn("Klaida:", result.stderr.decode())

    def test_stdin_preserves_binary_bytes(self):
        data = b"\x00a\xff\n"
        result = run("--stdin", data=data)
        self.assertEqual(result.returncode, 0)
        self.assertRegex(result.stdout.decode().strip(), r"^[0-9a-f]{64}$")


if __name__ == "__main__":
    unittest.main()
