#!/usr/bin/env python3
"""Update repository binaries and package current Windows and Mac builds."""

import argparse
import hashlib
import json
import os
from pathlib import Path
import shutil
import subprocess
import tempfile
import zipfile


PROGRAMS = (
    "railflow",
    "railflow_serial_monitor",
    "railflow_calibration_record",
    "railflow_camera_preview",
    "railflow_detect",
)


def zip_tree(folder, destination, prefix):
    with zipfile.ZipFile(destination, "w", zipfile.ZIP_DEFLATED, compresslevel=9) as archive:
        for path in sorted(folder.rglob("*")):
            relative = path.relative_to(folder).as_posix()
            if path.is_dir():
                if not any(path.iterdir()):
                    archive.write(path, prefix + relative + "/")
            else:
                archive.write(path, prefix + relative)
    with zipfile.ZipFile(destination) as archive:
        if archive.testzip() is not None:
            raise RuntimeError("Archive integrity check failed")


def publish(staged, destination):
    destination.parent.mkdir(parents=True, exist_ok=True)
    if destination.exists():
        digest = hashlib.sha256(destination.read_bytes()).hexdigest()[:16]
        backup = destination.parent / "backups" / (destination.name + "." + digest + ".bak")
        backup.parent.mkdir(exist_ok=True)
        if not backup.exists():
            shutil.copy2(destination, backup)
    temporary = destination.with_name(destination.name + ".updating")
    shutil.copy2(staged, temporary)
    os.replace(temporary, destination)


def batch(source, destination):
    text = source.read_text(encoding="utf-8")
    if not text.isascii():
        raise RuntimeError("Windows launcher must contain ASCII commands: " + str(source))
    destination.write_bytes(text.replace("\n", "\r\n").encode("ascii"))


def command_file(destination, body):
    destination.write_text("#!/bin/bash\n" + body, encoding="utf-8")
    destination.chmod(0o755)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--windows-bin", type=Path, required=True)
    parser.add_argument("--macos-arm64-bin", type=Path, required=True)
    parser.add_argument("--macos-intel-bin", type=Path, required=True)
    parser.add_argument("--output-dir", type=Path, required=True)
    arguments = parser.parse_args()
    root = Path(__file__).resolve().parent.parent
    arguments.output_dir.mkdir(parents=True, exist_ok=True)

    with tempfile.TemporaryDirectory(prefix="railflow-package-", dir=arguments.output_dir) as temporary:
        stage = Path(temporary)
        runtime = stage / "runtime"
        runtime.mkdir()
        mac = runtime / "bin/macos"
        mac.mkdir(parents=True)
        windows = runtime / "bin/windows-x64"
        windows.mkdir(parents=True)
        for name in PROGRAMS:
            shutil.copy2(arguments.windows_bin / (name + ".exe"), windows / (name + ".exe"))
            subprocess.run([
                "lipo", "-create", str(arguments.macos_arm64_bin / name),
                str(arguments.macos_intel_bin / name), "-output", str(mac / name),
            ], check=True)
            subprocess.run(["codesign", "--force", "--sign", "-", str(mac / name)], check=True)
            subprocess.run(["codesign", "--verify", str(mac / name)], check=True)
            architectures = subprocess.check_output(["lipo", "-archs", str(mac / name)], text=True).split()
            if set(architectures) != {"arm64", "x86_64"}:
                raise RuntimeError("Missing Mac architecture: " + name)
            (mac / name).chmod(0o755)

        # GitHub checkouts contain both platforms and use the same root launchers.
        shutil.copytree(runtime / "bin", root / "bin", dirs_exist_ok=True)
        binary_manifest = {
            "platforms": ["Windows x64", "macOS arm64", "macOS x86_64"],
            "macos_deployment_target": "11.0",
            "sha256": {},
        }
        for path in sorted((runtime / "bin").rglob("*")):
            if path.is_file():
                binary_manifest["sha256"][path.relative_to(runtime / "bin").as_posix()] = hashlib.sha256(path.read_bytes()).hexdigest()
        manifest_text = json.dumps(binary_manifest, indent=2) + "\n"
        (root / "bin/manifest.json").write_text(manifest_text)
        (runtime / "bin/manifest.json").write_text(manifest_text)

        shutil.copytree(root / "scripts", runtime / "scripts",
                        ignore=shutil.ignore_patterns("__pycache__", "package_release.py"))
        shutil.copytree(root / "docs", runtime / "docs")
        shutil.copytree(root / "firmware", runtime / "firmware")
        shutil.copytree(root / "third_party", runtime / "third_party")
        shutil.copy2(root / "START_RAILFLOW.command", runtime / "START_RAILFLOW.command")
        (runtime / "START_RAILFLOW.command").chmod(0o755)
        batch(root / "START_RAILFLOW.cmd", runtime / "START_RAILFLOW.cmd")
        aliases = {
            "windows-camera.cmd": "START_CAMERA.cmd",
            "windows-camera-probe.cmd": "CHECK_CAMERAS.cmd",
            "windows-camera-display-test.cmd": "TEST_DISPLAY.cmd",
            "windows-check.cmd": "TEST_WINDOWS.cmd",
        }
        for source, destination in aliases.items():
            batch(root / "scripts" / source, runtime / destination)
        batch(root / "TEST_SYSTEM.cmd", runtime / "TEST_SYSTEM.cmd")
        shutil.copy2(root / "TEST_SYSTEM.command", runtime / "TEST_SYSTEM.command")
        for path in (runtime / "scripts").glob("*.cmd"):
            batch(root / "scripts" / path.name, path)
        for filename, program_arguments in {
            "START_CAMERA.command": 'railflow_camera_preview "$@"',
            "CHECK_CAMERAS.command": "railflow_camera_preview --probe",
            "TEST_DISPLAY.command": "railflow_camera_preview --test-pattern --frames 150",
        }.items():
            command_file(runtime / filename,
                'root="$(cd "$(dirname "$0")" && pwd)"\n'
                'exec /bin/bash "$root/scripts/launch-posix.sh" ' + program_arguments + "\n")
        for folder in ("data/logs", "data/calibration", "camera-logs", "logs"):
            (runtime / folder).mkdir(parents=True, exist_ok=True)
        shutil.copy2(root / "docs/CROSS_PLATFORM_SETUP.md", runtime / "START_HERE.md")

        manifest = {
            "platforms": ["Windows x64", "macOS arm64", "macOS x86_64"],
            "macos_deployment_target": "11.0",
            "car_length_cm": 30,
            "car_width_cm": 15,
            "car_count": 3,
            "fsr_count": 9,
            "sha256": {},
        }
        for path in sorted(runtime.rglob("*")):
            if path.is_file():
                manifest["sha256"][path.relative_to(runtime).as_posix()] = hashlib.sha256(path.read_bytes()).hexdigest()
        (runtime / "MANIFEST.json").write_text(json.dumps(manifest, ensure_ascii=False, indent=2) + "\n")
        runtime_zip = stage / "runtime.zip"
        zip_tree(runtime, runtime_zip, "RailFlow/")

        source_zip = stage / "source.zip"
        excluded = {"build", ".git", ".DS_Store", "__MACOSX", "__pycache__"}
        old_binaries = {"railflow", "railflow_manual", "railflow_tests", "platform_tests"}
        with zipfile.ZipFile(source_zip, "w", zipfile.ZIP_DEFLATED, compresslevel=9) as archive:
            for path in sorted(root.rglob("*")):
                relative = path.relative_to(root)
                if any(part in excluded for part in relative.parts):
                    continue
                if relative.parts[0] in {"logs", "camera-logs", "system-checks"} or relative.parts[0].startswith("windows-check-"):
                    continue
                if len(relative.parts) == 1 and relative.name in old_binaries:
                    continue
                if path.is_file():
                    archive.write(path, "railflow/" + relative.as_posix())
        with zipfile.ZipFile(source_zip) as archive:
            if archive.testzip() is not None:
                raise RuntimeError("Source archive integrity check failed")

        for name in ("RailFlow-Windows-Mac-2026-11-19.zip", "RailFlow-Windows-x64-2026-11-19-r3.zip"):
            publish(runtime_zip, arguments.output_dir / name)
        publish(source_zip, arguments.output_dir / "RailFlow-source-2026-11-19-r3.zip")
        print("Updated cross-platform runtime, legacy r3 runtime, and source ZIPs")


if __name__ == "__main__":
    main()
