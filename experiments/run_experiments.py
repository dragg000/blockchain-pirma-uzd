#!/usr/bin/env python3
"""Atkuriami maišos generatoriaus eksperimentai."""

import csv
import hashlib
import json
import pathlib
import random
import struct
import subprocess
import tempfile
import time


ROOT = pathlib.Path(__file__).resolve().parents[1]
OUT = ROOT / "results"
BUILD = ROOT / "build"
BINARY = BUILD / "hash_generator"
BENCHMARK = BUILD / "benchmark"
SEED = 20261007
ALPHABET = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789 .,;:!?-_"
LENGTHS = (10, 100, 500, 1000)


def build():
    BUILD.mkdir(exist_ok=True)
    wrapper = BUILD / "batch_wrapper.cpp"
    wrapper.write_text(
        '#define main original_main\n#include "../src/main.cpp"\n#undef main\n'
        '#include <sstream>\n'
        'int main() { std::string line; while (std::getline(std::cin, line)) { '
        'std::vector<uint8_t> bytes; for (size_t i=0; i<line.size(); i += 2) '
        'bytes.push_back(static_cast<uint8_t>(std::stoi(line.substr(i, 2), nullptr, 16))); '
        'std::string input(bytes.begin(), bytes.end()); std::cout << toHex(hash(input)) << "\\n"; } }\n',
        encoding="utf-8",
    )
    subprocess.run(["c++", "-std=c++20", "-O2", "-Wall", "-Wextra", "-pedantic",
                    str(wrapper), "-o", str(BINARY)], cwd=BUILD, check=True)


def hash_one(data):
    return hash_batch([data])[0]


def hash_batch(items):
    payload = "".join(item.hex() + "\n" for item in items).encode()
    result = subprocess.run([str(BINARY)], input=payload,
                            capture_output=True, check=True)
    return [bytes.fromhex(line) for line in result.stdout.decode().splitlines()]


def random_bytes(rng, length):
    return "".join(rng.choice(ALPHABET) for _ in range(length)).encode()


def write_csv(path, header, rows):
    with path.open("w", newline="", encoding="utf-8") as stream:
        writer = csv.writer(stream)
        writer.writerow(header)
        writer.writerows(rows)


def make_svg(path, title, x_label, y_label, points):
    width, height = 900, 520
    left, right, top, bottom = 90, 30, 55, 75
    plot_w, plot_h = width - left - right, height - top - bottom
    xs = [float(x) for x, _ in points]
    ys = [float(y) for _, y in points]
    x_min, x_max = min(xs), max(xs)
    y_max = max(ys) * 1.1 or 1

    def px(x):
        return left + (x - x_min) / (x_max - x_min or 1) * plot_w

    def py(y):
        return top + plot_h - y / y_max * plot_h

    polyline = " ".join(f"{px(x):.1f},{py(y):.1f}" for x, y in points)
    labels = []
    for x, y in points:
        labels.append(
            f'<circle cx="{px(x):.1f}" cy="{py(y):.1f}" r="4" fill="#2457a6"/>'
        )
    svg = f"""<svg xmlns="http://www.w3.org/2000/svg" width="{width}" height="{height}">
<rect width="100%" height="100%" fill="white"/>
<text x="{width/2}" y="28" text-anchor="middle" font-size="20">{title}</text>
<line x1="{left}" y1="{top}" x2="{left}" y2="{top+plot_h}" stroke="black"/>
<line x1="{left}" y1="{top+plot_h}" x2="{left+plot_w}" y2="{top+plot_h}" stroke="black"/>
<polyline points="{polyline}" fill="none" stroke="#2457a6" stroke-width="2"/>
{''.join(labels)}
<text x="{width/2}" y="{height-20}" text-anchor="middle">{x_label}</text>
<text x="20" y="{height/2}" text-anchor="middle" transform="rotate(-90 20 {height/2})">{y_label}</text>
</svg>
"""
    path.write_text(svg, encoding="utf-8")



def correctness():
    inputs = [
        b"", b"a", b"b", b"abc", b"abc\n", b"  abc  ",
        "Lietuva".encode(), "ąžuolas €".encode(), bytes(range(256)),
    ]
    rows = []
    for data in inputs:
        digest = hash_one(data).hex()
        rows.append({"bytes": len(data), "hex_length": len(digest),
                     "digest": digest, "repeat_equal": digest == hash_one(data).hex()})
    (OUT / "correctness.json").write_text(json.dumps(rows, indent=2), encoding="utf-8")


def benchmark():
    rng = random.Random(SEED)
    lines = ["eilutė %04d: %s\n" % (i, "".join(rng.choice(ALPHABET) for _ in range(70)))
             for i in range(2048)]
    corpus = "".join(lines).encode()
    rows = []
    line_count = 1
    while line_count <= len(lines):
        data = "".join(lines[:line_count]).encode()
        samples = []
        for _ in range(5):
            start = time.perf_counter_ns()
            hash_batch([data])
            samples.append((time.perf_counter_ns() - start) / 1000)
        size, average = len(data), sum(samples) / len(samples)
        minimum, maximum = min(samples), max(samples)
        rows.append((int(size), line_count, average, minimum, maximum))
        line_count *= 2
    data = corpus
    samples = []
    for _ in range(5):
        start = time.perf_counter_ns()
        hash_batch([data])
        samples.append((time.perf_counter_ns() - start) / 1000)
    size, average = len(data), sum(samples) / len(samples)
    minimum, maximum = min(samples), max(samples)
    rows.append((int(size), len(lines), average, minimum, maximum))
    write_csv(OUT / "benchmark.csv",
              ["bytes", "lines", "average_us", "minimum_us", "maximum_us"], rows)
    make_svg(OUT / "benchmark.svg", "Maišos skaičiavimo sparta",
             "Įvesties dydis baitais", "Laikas vienai maišai (µs)",
             [(row[0], row[2]) for row in rows])


def collisions():
    rng = random.Random(SEED)
    rows = []
    for length in LENGTHS:
        pairs = [(random_bytes(rng, length), random_bytes(rng, length))
                 for _ in range(100_000)]
        pair_hashes = hash_batch([item for pair in pairs for item in pair])
        pair_collisions = sum(
            pair_hashes[i] == pair_hashes[i + 1] and pairs[i // 2][0] != pairs[i // 2][1]
            for i in range(0, len(pair_hashes), 2)
        )
        values = {}
        all_inputs = [item for pair in pairs for item in pair]
        for index, data in enumerate(all_inputs):
            values.setdefault(pair_hashes[index], []).append((index, data))
        groups = sum(len(items) > 1 and len({data for _, data in items}) > 1
                     for items in values.values())
        rows.append((length, 100_000, pair_collisions, 200_000, groups))
    structured = [b"a" * 10, b"b" * 10, b"ab" * 5, b"ba" * 5, b" " * 10]
    structured_hashes = hash_batch(structured)
    rows.append(("structured", len(structured),
                 len(structured_hashes) - len(set(structured_hashes)), len(structured), 0))
    write_csv(OUT / "collisions.csv",
              ["length", "pairs", "pair_collisions", "set_inputs", "collision_groups"], rows)


def avalanche():
    rng = random.Random(SEED)
    rows = []
    for length in LENGTHS:
        bit_values, hex_values = [], []
        inputs = []
        for _ in range(25_000):
            original = bytearray(random_bytes(rng, length))
            changed = original[:]
            position = rng.randrange(length)
            old = changed[position]
            choices = [c for c in range(32, 127) if c != old]
            changed[position] = rng.choice(choices)
            inputs.extend((bytes(original), bytes(changed)))
        digests = hash_batch(inputs)
        for i in range(0, len(digests), 2):
            first, second = digests[i], digests[i + 1]
            bits = sum((a ^ b).bit_count() for a, b in zip(first, second))
            hex_digits = sum(a != b for a, b in zip(first.hex(), second.hex()))
            bit_percent = bits / 256 * 100
            hex_percent = hex_digits / 64 * 100
            bit_values.append(bit_percent)
            hex_values.append(hex_percent)
        rows.append((length, min(bit_values), max(bit_values), sum(bit_values) / len(bit_values),
                     min(hex_values), max(hex_values), sum(hex_values) / len(hex_values)))
    write_csv(OUT / "avalanche.csv",
              ["length", "bit_min_pct", "bit_max_pct", "bit_average_pct",
               "hex_min_pct", "hex_max_pct", "hex_average_pct"], rows)


def preimage():
    candidates = [f"{i:04d}".encode() for i in range(10_000)]
    salt = b"VU-2026"
    targets = [(b"0420", b""), (b"0420", salt)]
    rows = []
    for target, public_salt in targets:
        target_hash = hash_one(target + public_salt)
        start = time.perf_counter()
        matches = [candidate.decode() for candidate, digest in
                   zip(candidates, hash_batch([candidate + public_salt for candidate in candidates]))
                   if digest == target_hash]
        elapsed = time.perf_counter() - start
        rows.append((target.decode(), public_salt.decode(), len(candidates),
                     len(matches), ";".join(matches), elapsed))
    write_csv(OUT / "preimage.csv",
              ["target", "salt", "attempts", "matches", "matching_candidates", "seconds"], rows)


def main():
    OUT.mkdir(exist_ok=True)
    build()
    correctness()
    benchmark()
    collisions()
    avalanche()
    preimage()
    print(f"Rezultatai išsaugoti: {OUT}")


if __name__ == "__main__":
    main()
