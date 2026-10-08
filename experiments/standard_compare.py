#!/usr/bin/env python3
"""Palygina v0.1 su MD5, SHA-1 ir SHA-256 vienodomis įvestimis."""

import csv
import hashlib
import pathlib
import random
import statistics
import subprocess
import time


ROOT = pathlib.Path(__file__).resolve().parents[1]
OUT = ROOT / "results"
BUILD = ROOT / "build"
HELPER = BUILD / "standard_compare"
SEED = 20261007
LENGTHS = (10, 100, 500, 1000)
ALPHABET = b"abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789 .,;:!?-_"


def build():
    BUILD.mkdir(exist_ok=True)
    subprocess.run(
        ["c++", "-std=c++20", "-O2", "-Wall", "-Wextra", "-pedantic",
         "experiments/standard_compare.cpp", "-o", str(HELPER)],
        cwd=ROOT, check=True,
    )


def custom_hashes(inputs):
    payload = "".join(data.hex() + "\n" for data in inputs).encode()
    output = subprocess.run([str(HELPER)], input=payload, capture_output=True,
                            check=True).stdout.decode().splitlines()
    return [bytes.fromhex(value) for value in output]


def bit_difference(first, second):
    return sum((a ^ b).bit_count() for a, b in zip(first, second))


def hex_difference(first, second):
    return sum(a != b for a, b in zip(first.hex(), second.hex()))


def standard_digest(name, data):
    return hashlib.new(name, data).digest()


def benchmark(name, inputs):
    for data in inputs[:3]:
        standard_digest(name, data)
    measurements = []
    for _ in range(5):
        start = time.perf_counter_ns()
        sink = 0
        for data in inputs:
            sink ^= standard_digest(name, data)[0]
        elapsed = (time.perf_counter_ns() - start) / 1000
        measurements.append((elapsed / len(inputs), sink))
    return statistics.mean(value for value, _ in measurements), min(
        value for value, _ in measurements
    ), max(value for value, _ in measurements)


def custom_benchmark(inputs):
    payload = "".join(data.hex() + "\n" for data in inputs).encode()
    measurements = []
    for _ in range(5):
        result = subprocess.run([str(HELPER), "--benchmark"], input=payload,
                                capture_output=True, check=True)
        total, _ = result.stdout.decode().split(",")
        measurements.append(float(total) / len(inputs))
    return statistics.mean(measurements), min(measurements), max(measurements)


def main():
    OUT.mkdir(exist_ok=True)
    build()
    rng = random.Random(SEED)
    inputs = [bytes(rng.choice(ALPHABET) for _ in range(length))
              for length in LENGTHS for _ in range(250)]
    changed = []
    for data in inputs:
        other = bytearray(data)
        position = rng.randrange(len(other))
        old = other[position]
        replacement = rng.choice([value for value in ALPHABET if value != old])
        other[position] = replacement
        changed.append(bytes(other))

    all_digests = {"v0.1": custom_hashes(inputs), "MD5": [],
                   "SHA-1": [], "SHA-256": []}
    for name in ("md5", "sha1", "sha256"):
        label = name.upper().replace("SHA1", "SHA-1").replace("SHA256", "SHA-256")
        all_digests[label] = [standard_digest(name, data) for data in inputs]

    speed_rows = []
    speed_rows.append(("v0.1", *custom_benchmark(inputs)))
    for name in ("md5", "sha1", "sha256"):
        label = name.upper().replace("SHA1", "SHA-1").replace("SHA256", "SHA-256")
        speed_rows.append((label, *benchmark(name, inputs)))
    with (OUT / "standard_speed.csv").open("w", newline="", encoding="utf-8") as stream:
        writer = csv.writer(stream)
        writer.writerow(["algorithm", "average_us_per_hash", "minimum_us", "maximum_us"])
        writer.writerows(speed_rows)

    avalanche_rows = []
    custom_changed = custom_hashes(changed)
    for label, digests in all_digests.items():
        if label == "v0.1":
            second = custom_changed
        else:
            algorithm = {"MD5": "md5", "SHA-1": "sha1", "SHA-256": "sha256"}[label]
            second = [standard_digest(algorithm, data) for data in changed]
        bit_values = [100 * bit_difference(a, b) / (len(a) * 8)
                      for a, b in zip(digests, second)]
        hex_values = [100 * hex_difference(a, b) / len(a.hex())
                      for a, b in zip(digests, second)]
        avalanche_rows.append((
            label, len(digests[0]) * 8, min(bit_values), max(bit_values),
            statistics.mean(bit_values), min(hex_values), max(hex_values),
            statistics.mean(hex_values),
        ))
    with (OUT / "standard_avalanche.csv").open("w", newline="", encoding="utf-8") as stream:
        writer = csv.writer(stream)
        writer.writerow(["algorithm", "bits", "bit_min_pct", "bit_max_pct",
                         "bit_average_pct", "hex_min_pct", "hex_max_pct",
                         "hex_average_pct"])
        writer.writerows(avalanche_rows)


if __name__ == "__main__":
    main()
