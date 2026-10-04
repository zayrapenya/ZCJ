"""Helpers shared by the benchmarks.

Every benchmark writes a CSV with the same columns so the results of the
different languages (Python, Java, ...) can be merged and compared:

    language,benchmark,structure,n_books,metric,value
"""

import csv
import os
import shutil
import tempfile
import time
from pathlib import Path

from search_engine.config import BASE_DIR, SAMPLE_DIR
from search_engine.downloader import split_book

RESULTS_DIR = BASE_DIR / "benchmarks" / "results"
# Scratch data is written to the system temp folder by default so that synced folders
# (OneDrive, Dropbox...) do not interfere with the measurements. Override with BENCH_WORKSPACE.
WORKSPACE = Path(os.environ.get("BENCH_WORKSPACE", Path(tempfile.gettempdir()) / "stage1_bench_python"))
FIRST_SYNTHETIC_ID = 100000


def load_sample_books() -> list[tuple[str, str]]:
    """(header, body) of every book in sample_data/, always in the same order."""
    books = []
    for path in sorted(SAMPLE_DIR.glob("pg*.txt"), key=lambda p: int(p.stem[2:])):
        books.append(split_book(path.read_text(encoding="utf-8")))
    return books


def synthetic_books(n: int) -> list[tuple[int, str, str]]:
    """n books (id, header, body) built by cycling over the sample dataset."""
    sample = load_sample_books()
    return [(FIRST_SYNTHETIC_ID + i, *sample[i % len(sample)]) for i in range(n)]


def fresh_workspace(name: str) -> Path:
    path = WORKSPACE / name
    shutil.rmtree(path, ignore_errors=True)
    path.mkdir(parents=True)
    return path


class Timer:
    def __enter__(self):
        self.start = time.perf_counter()
        return self

    def __exit__(self, *exc):
        self.seconds = time.perf_counter() - self.start


class Results:
    def __init__(self, benchmark: str):
        self.benchmark = benchmark
        self.rows = []

    def add(self, structure: str, n_books: int, metric: str, value: float):
        self.rows.append(["python", self.benchmark, structure, n_books, metric, round(value, 6)])
        print(f"  {structure:<13} n={n_books:<6} {metric:<28} {value:.6f}")

    def save(self) -> Path:
        RESULTS_DIR.mkdir(parents=True, exist_ok=True)
        path = RESULTS_DIR / f"python_{self.benchmark}.csv"
        with open(path, "w", newline="", encoding="utf-8") as f:
            writer = csv.writer(f)
            writer.writerow(["language", "benchmark", "structure", "n_books", "metric", "value"])
            writer.writerows(self.rows)
        print(f"Results saved to {path}")
        return path
