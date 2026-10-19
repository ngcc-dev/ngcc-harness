#!/usr/bin/env python3
"""Focused tests for the public reproducer dispatcher's execution boundary."""
from __future__ import annotations

import csv
import hashlib
import importlib.util
import subprocess
import tempfile
import unittest
from pathlib import Path
from unittest import mock


PACKAGE = Path(__file__).resolve().parents[1]
SPEC = importlib.util.spec_from_file_location("nist_reproduce", PACKAGE / "reproduce.py")
if SPEC is None or SPEC.loader is None:  # pragma: no cover - import machinery failure
    raise RuntimeError("cannot load reproduce.py")
REPRODUCE = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(REPRODUCE)
FETCH_SPEC = importlib.util.spec_from_file_location(
    "nist_fetch_submissions", PACKAGE / "tools/fetch_submissions.py",
)
if FETCH_SPEC is None or FETCH_SPEC.loader is None:  # pragma: no cover
    raise RuntimeError("cannot load fetch_submissions.py")
FETCH = importlib.util.module_from_spec(FETCH_SPEC)
FETCH_SPEC.loader.exec_module(FETCH)
SYNC_SPEC = importlib.util.spec_from_file_location(
    "nist_sync", PACKAGE / "tools/sync.py",
)
if SYNC_SPEC is None or SYNC_SPEC.loader is None:  # pragma: no cover
    raise RuntimeError("cannot load sync.py")
SYNC = importlib.util.module_from_spec(SYNC_SPEC)
SYNC_SPEC.loader.exec_module(SYNC)
SNOVA_SPEC = importlib.util.spec_from_file_location(
    "nist_snova_6", PACKAGE / "SNOVA/reproduce_snova_6.py",
)
if SNOVA_SPEC is None or SNOVA_SPEC.loader is None:  # pragma: no cover
    raise RuntimeError("cannot load reproduce_snova_6.py")
SNOVA = importlib.util.module_from_spec(SNOVA_SPEC)
SNOVA_SPEC.loader.exec_module(SNOVA)


class FakeResponse:
    def __init__(self, content: bytes):
        self.content = content

    def __enter__(self):
        return self

    def __exit__(self, *unused) -> None:
        return None

    def raise_for_status(self) -> None:
        return None

    def iter_content(self, unused_size: int):
        yield self.content


class DispatcherTests(unittest.TestCase):
    def test_fetcher_uses_official_round3_page(self) -> None:
        self.assertEqual(
            FETCH.PAGE,
            "https://csrc.nist.gov/Projects/pqc-dig-sig/round-3-additional-signatures",
        )

    def test_child_executes_verified_bytes_after_path_changes(self) -> None:
        with tempfile.TemporaryDirectory() as temporary:
            script = Path(temporary) / "witness.py"
            verified = b"print('verified bytes')\n"
            script.write_bytes(verified)
            command = REPRODUCE.isolated_command(script, [])
            script.write_text("print('changed file')\n", encoding="utf-8")
            result = subprocess.run(command, input=verified, capture_output=True, check=False)
            self.assertEqual(result.returncode, 0, result.stderr.decode())
            self.assertEqual(result.stdout, b"verified bytes\n")

    def test_sibling_cannot_shadow_standard_library(self) -> None:
        with tempfile.TemporaryDirectory() as temporary:
            directory = Path(temporary)
            script = directory / "witness.py"
            script.write_text("import fractions; print(fractions.Fraction(1, 2))\n")
            (directory / "fractions.py").write_text("raise RuntimeError('shadowed')\n")
            content = script.read_bytes()
            result = subprocess.run(
                REPRODUCE.isolated_command(script, []),
                input=content,
                capture_output=True,
                check=False,
            )
            self.assertEqual(result.returncode, 0, result.stderr.decode())
            self.assertEqual(result.stdout, b"1/2\n")

    def test_inventory_rejects_unlisted_python(self) -> None:
        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary)
            with (root / "source-manifest.csv").open("w", encoding="utf-8", newline="") as target:
                writer = csv.writer(target, lineterminator="\n")
                writer.writerow(("SourcePath", "SourceSHA256", "OutputPath", "OutputSHA256"))
            (root / "hashlib.py").write_text("raise RuntimeError('shadowed')\n")
            original = REPRODUCE.ROOT
            REPRODUCE.ROOT = root
            try:
                with self.assertRaisesRegex(ValueError, "unlisted Python file"):
                    REPRODUCE.verify_public_inputs()
            finally:
                REPRODUCE.ROOT = original

    def test_failed_pin_removes_target_and_partial(self) -> None:
        with tempfile.TemporaryDirectory() as temporary:
            target = Path(temporary) / "candidate.zip"
            target.write_bytes(b"stale")
            with mock.patch.object(FETCH.requests, "get", return_value=FakeResponse(b"bad")):
                with self.assertRaisesRegex(ValueError, "differs from pinned provenance"):
                    FETCH.fetch("https://example.invalid/candidate.zip", target, "0" * 64, 3)
            self.assertFalse(target.exists())
            self.assertFalse(target.with_suffix(".zip.part").exists())

    def test_reset_directory_removes_stale_tree(self) -> None:
        with tempfile.TemporaryDirectory() as temporary:
            target = Path(temporary) / "extracted" / "SQIsign"
            target.mkdir(parents=True)
            (target / "stale.txt").write_text("stale\n", encoding="utf-8")
            FETCH.reset_directory(target)
            self.assertTrue(target.is_dir())
            self.assertEqual(list(target.iterdir()), [])

    def test_reset_directory_unlinks_symlink_without_touching_victim(self) -> None:
        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary)
            victim = root / "victim"
            victim.mkdir()
            marker = victim / "keep.txt"
            marker.write_text("keep\n", encoding="utf-8")
            target = root / "extracted"
            target.symlink_to(victim, target_is_directory=True)
            FETCH.reset_directory(target)
            self.assertFalse(target.is_symlink())
            self.assertTrue(target.is_dir())
            self.assertEqual(marker.read_text(encoding="utf-8"), "keep\n")

    def test_sync_rejects_imported_untracked_input(self) -> None:
        manifest = [{
            "SourcePath": "SQIsign/analysis/ignored.py",
            "SourceSHA256": "0" * 64,
            "OutputPath": "SQIsign/analysis/ignored.py",
            "OutputSHA256": "0" * 64,
        }]
        tracked = subprocess.CompletedProcess(
            ["git", "ls-files"], 0, stdout=b"README.md\0", stderr=b"",
        )
        with mock.patch.object(SYNC.subprocess, "run", return_value=tracked):
            with self.assertRaisesRegex(SystemExit, "refusing untracked publication input"):
                SYNC.validate_tracked_inputs(Path("/source"), manifest, False)
            SYNC.validate_tracked_inputs(Path("/source"), manifest, True)

    def test_snova_children_use_isolated_python(self) -> None:
        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary)
            library = root / "SNOVA/lib/test.so"
            library.parent.mkdir(parents=True)
            library.write_bytes(b"test library")
            work = root / "work"
            work.mkdir()
            commands: list[list[str]] = []

            def fake_run(command, **unused):
                commands.append(command)
                if "--child-verify" in command:
                    case = Path(command[-1])
                    case.joinpath("verify.json").write_text(
                        '{"first_rc": -1, "first_message_matches": false, '
                        '"second_rc": 0, "second_message_matches": true}\n',
                        encoding="utf-8",
                    )
                return subprocess.CompletedProcess(command, 0, "", "")

            original_root = SNOVA.ROOT
            SNOVA.ROOT = root
            try:
                with mock.patch.object(SNOVA.subprocess, "run", side_effect=fake_run):
                    SNOVA.run_one(
                        {"parameter": "test", "library": "SNOVA/lib/test.so"}, work,
                    )
            finally:
                SNOVA.ROOT = original_root
            self.assertEqual(len(commands), 2)
            for command in commands:
                self.assertEqual(command[1], "-I")

    def test_build_failure_is_failure(self) -> None:
        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary)
            script = root / "UOV/reproduce_uov_99.py"
            script.parent.mkdir()
            script.write_text("print('must not run')\n", encoding="utf-8")
            (root / "UOV/submission").mkdir()
            script_hash = hashlib.sha256(script.read_bytes()).hexdigest()
            with (root / "reproducers.csv").open("w", encoding="utf-8", newline="") as target:
                writer = csv.writer(target, lineterminator="\n")
                writer.writerow(REPRODUCE.REPRODUCER_FIELDS)
                writer.writerow(("UOV-99", "UOV/reproduce_uov_99.py", script_hash, "yes", "UOV", "libs", "make"))
            with (root / "source-manifest.csv").open("w", encoding="utf-8", newline="") as target:
                writer = csv.writer(target, lineterminator="\n")
                writer.writerow(("SourcePath", "SourceSHA256", "OutputPath", "OutputSHA256"))
                writer.writerow(("UOV/reproduce_uov_99.py", script_hash, "UOV/reproduce_uov_99.py", script_hash))
            original_root, original_argv = REPRODUCE.ROOT, REPRODUCE.sys.argv
            REPRODUCE.ROOT = root
            REPRODUCE.sys.argv = ["reproduce.py", "UOV-99"]
            try:
                failed = subprocess.CompletedProcess(["make"], 2)
                with mock.patch.object(REPRODUCE.subprocess, "run", return_value=failed):
                    self.assertEqual(REPRODUCE.main(), 1)
            finally:
                REPRODUCE.ROOT = original_root
                REPRODUCE.sys.argv = original_argv


if __name__ == "__main__":
    unittest.main()
