#!/usr/bin/env python3
"""Palygina pažymėtą v0.1 bazę su dabartine v0.11 versija."""

import csv
import pathlib
import random
import subprocess
import time
from xml.sax.saxutils import escape

ROOT = pathlib.Path(__file__).resolve().parents[1]
BUILD = ROOT / "build"
RESULTS = ROOT / "results"
ALPHABET = b"abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789 .,;:!?-_"
SEED = 20261008


def make_wrapper(source, output, label):
    wrapper = BUILD / f"{label}_wrapper.cpp"
    wrapper.write_text(
        '#define main original_main\n'
        f'#include "{source.name}"\n'
        '#undef main\n'
        '#include <iomanip>\n#include <iostream>\n#include <sstream>\n'
        '#include <string>\n#include <vector>\n'
        'std::string hex(const std::vector<uint8_t>& bytes) { std::ostringstream out; '
        'out << std::hex << std::setfill("0"[0]); for (auto b: bytes) '
        'out << std::setw(2) << static_cast<unsigned>(b); return out.str(); }\n'
        'std::vector<uint8_t> unhex(const std::string& s) { std::vector<uint8_t> b; '
        'for (size_t i=0;i<s.size();i+=2) b.push_back(static_cast<uint8_t>('
        'std::stoi(s.substr(i,2), nullptr, 16))); return b; }\n'
        'int main() { std::string line; while(std::getline(std::cin,line)) { '
        'auto bytes=unhex(line); std::string text(bytes.begin(),bytes.end()); '
        'std::cout << hex(hash(text)) << "\\n"; } }\n',
        encoding="utf-8",
    )
    subprocess.run(["c++", "-std=c++20", "-O2", str(wrapper), "-o", str(output)],
                   check=True, cwd=BUILD)


def compile_versions():
    BUILD.mkdir(exist_ok=True)
    baseline = BUILD / "v01_source.cpp"
    baseline.write_bytes(subprocess.check_output(
        ["git", "show", "refs/tags/v0.1:src/main.cpp"], cwd=ROOT))
    current = BUILD / "v011_source.cpp"
    current.write_text((ROOT / "src/main.cpp").read_text(), encoding="utf-8")
    make_wrapper(baseline, BUILD / "v01_hash", "v01")
    make_wrapper(current, BUILD / "v011_hash", "v011")


def run(binary, inputs):
    payload = "".join(item.hex() + "\n" for item in inputs).encode()
    return [bytes.fromhex(line) for line in subprocess.check_output(
        [str(binary)], input=payload).decode().splitlines()]


def write_bar_chart(path, title, ylabel, values, decimals=3):
    width, height = 900, 500
    plot_left, plot_top, plot_width, plot_height = 100, 75, 730, 320
    maximum = max(values) * 1.2
    bar_width = 180
    gap = 170
    colors = ("#4472c4", "#ed7d31")
    svg = [
        f'<svg xmlns="http://www.w3.org/2000/svg" width="{width}" height="{height}" '
        f'viewBox="0 0 {width} {height}">',
        '<rect width="100%" height="100%" fill="white"/>',
        f'<text x="{width / 2}" y="35" text-anchor="middle" font-family="sans-serif" '
        f'font-size="24" font-weight="bold">{escape(title)}</text>',
        f'<text x="25" y="{plot_top + plot_height / 2}" text-anchor="middle" '
        f'font-family="sans-serif" font-size="16" transform="rotate(-90 25 '
        f'{plot_top + plot_height / 2})">{escape(ylabel)}</text>',
        f'<line x1="{plot_left}" y1="{plot_top + plot_height}" '
        f'x2="{plot_left + plot_width}" y2="{plot_top + plot_height}" '
        'stroke="#333"/>',
        f'<line x1="{plot_left}" y1="{plot_top}" x2="{plot_left}" '
        f'y2="{plot_top + plot_height}" stroke="#333"/>',
    ]
    for index, (label, value) in enumerate(zip(("v0.1", "v0.11"), values)):
        x = plot_left + 130 + index * (bar_width + gap)
        bar_height = value / maximum * plot_height
        y = plot_top + plot_height - bar_height
        svg.extend([
            f'<rect x="{x}" y="{y:.2f}" width="{bar_width}" height="{bar_height:.2f}" '
            f'fill="{colors[index]}"/>',
            f'<text x="{x + bar_width / 2}" y="{y - 10:.2f}" text-anchor="middle" '
            f'font-family="sans-serif" font-size="16">{value:.{decimals}f}</text>',
            f'<text x="{x + bar_width / 2}" y="{plot_top + plot_height + 30}" '
            'text-anchor="middle" font-family="sans-serif" font-size="17">'
            f'{label}</text>',
        ])
    svg.append("</svg>\n")
    path.write_text("\n".join(svg), encoding="utf-8")


def main():
    RESULTS.mkdir(exist_ok=True)
    compile_versions()
    rng = random.Random(SEED)
    inputs = [bytes(rng.choice(ALPHABET) for _ in range(length))
              for length in (10, 100, 500, 1000) for _ in range(250)]
    changed = []
    for item in inputs:
        value = bytearray(item)
        position = rng.randrange(len(value))
        value[position] = rng.choice([x for x in ALPHABET if x != value[position]])
        changed.append(bytes(value))

    rows = []
    for label in ("v01", "v011"):
        binary = BUILD / f"{label}_hash"
        outputs = run(binary, inputs)
        changed_outputs = run(binary, changed)
        timings = []
        for _ in range(5):
            start = time.perf_counter_ns()
            run(binary, inputs)
            timings.append((time.perf_counter_ns() - start) / 1000 / len(inputs))
        bit_values = [
            100 * sum((a ^ b).bit_count() for a, b in zip(first, second)) / 256
            for first, second in zip(outputs, changed_outputs)
        ]
        hex_values = [
            100 * sum(a != b for a, b in zip(first.hex(), second.hex())) / 64
            for first, second in zip(outputs, changed_outputs)
        ]
        rows.append((label, len(inputs), sum(timings) / len(timings),
                     min(timings), max(timings), min(bit_values),
                     max(bit_values), sum(bit_values) / len(bit_values),
                     min(hex_values), max(hex_values),
                     sum(hex_values) / len(hex_values)))
    with (RESULTS / "v01_v011_comparison.csv").open("w", newline="") as stream:
        writer = csv.writer(stream)
        writer.writerow(["version", "inputs", "average_us", "minimum_us", "maximum_us",
                         "bit_min_pct", "bit_max_pct", "bit_average_pct",
                         "hex_min_pct", "hex_max_pct", "hex_average_pct"])
        writer.writerows(rows)
    write_bar_chart(
        RESULTS / "v01_v011_speed.svg",
        "v0.1 ir v0.11 spartos palyginimas",
        "Vidutinis laikas vienai maišai (µs)",
        [row[2] for row in rows],
    )
    write_bar_chart(
        RESULTS / "v01_v011_avalanche.svg",
        "v0.1 ir v0.11 lavinos efekto palyginimas",
        "Pakeistų išvesties bitų dalis (%)",
        [row[7] for row in rows],
        decimals=3,
    )


if __name__ == "__main__":
    main()
