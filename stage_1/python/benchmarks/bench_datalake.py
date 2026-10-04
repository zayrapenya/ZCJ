"""Datalake structure benchmark (section 3.1 of the guide).

For each layout (time, book, range) it measures:
  * write throughput      books split + stored per second
  * lookup cost           average time to locate header + body of a book
  * incremental detection time to find the books not yet indexed
  * recovery              re-running after an interruption: duplicated / lost books
  * storage overhead      files, directories and bytes

Run from stage_1/python:  python -m benchmarks.bench_datalake --books 100 200
"""

import argparse
import random
from datetime import datetime, timedelta

from search_engine.control import ControlFile
from search_engine.datalake import LAYOUTS
from search_engine.downloader import split_book

from .common import Results, Timer, fresh_workspace, load_sample_books, synthetic_books

BOOKS_PER_HOUR = 10  # simulated ingestion rate for the time-based layout
LOOKUPS = 500


def ingest(datalake, control, books, start_time):
    for i, (book_id, header, body) in enumerate(books):
        datalake.save(book_id, header, body, now=start_time + timedelta(hours=i // BOOKS_PER_HOUR))
        control.add(book_id)


def run(sizes: list[int]):
    results = Results("datalake")
    raw_sample = [f"{h}\n*** START OF THE PROJECT GUTENBERG EBOOK X ***\n{b}\n*** END OF THE PROJECT GUTENBERG EBOOK X ***"
                  for h, b in load_sample_books()]
    start_time = datetime(2025, 9, 25, 0)

    for n in sizes:
        books = synthetic_books(n)
        ids = [b[0] for b in books]
        rng = random.Random(42)
        lookup_ids = [rng.choice(ids) for _ in range(LOOKUPS)]

        for name, layout in LAYOUTS.items():
            workspace = fresh_workspace(f"datalake_{name}")
            datalake = layout(workspace / "datalake")
            control = ControlFile(workspace / "control" / "downloaded_books.txt")

            # Write throughput (includes splitting the raw text, as in the real pipeline)
            with Timer() as t:
                for i, (book_id, _, _) in enumerate(books):
                    header, body = split_book(raw_sample[i % len(raw_sample)])
                    datalake.save(book_id, header, body, now=start_time + timedelta(hours=i // BOOKS_PER_HOUR))
                    control.add(book_id)
            results.add(name, n, "write_books_per_sec", n / t.seconds)

            # Lookup cost
            with Timer() as t:
                for book_id in lookup_ids:
                    assert datalake.locate(book_id) is not None
            results.add(name, n, "lookup_avg_ms", 1000 * t.seconds / LOOKUPS)

            # Incremental processing: half of the books are already indexed
            indexed = ControlFile(workspace / "control" / "indexed_books.txt")
            for book_id in ids[: n // 2]:
                indexed.add(book_id)
            with Timer() as t:
                pending_control = control.read() - indexed.read()
            results.add(name, n, "incremental_control_ms", 1000 * t.seconds)
            with Timer() as t:
                pending_scan = datalake.book_ids() - indexed.read()
            results.add(name, n, "incremental_scan_ms", 1000 * t.seconds)
            assert pending_control == pending_scan

            # Storage overhead
            stats = datalake.storage_stats()
            results.add(name, n, "files", stats["files"])
            results.add(name, n, "dirs", stats["dirs"])
            results.add(name, n, "megabytes", stats["bytes"] / 1e6)

        # Recovery: interrupt after 60% of the books, restart one hour later
        for name, layout in LAYOUTS.items():
            workspace = fresh_workspace(f"recovery_{name}")
            datalake = layout(workspace / "datalake")
            control = ControlFile(workspace / "control" / "downloaded_books.txt")
            cut = int(n * 0.6)
            ingest(datalake, control, books[:cut], start_time)
            # crash: the last book was written to disk but not registered in the control file
            last_id, header, body = books[cut]
            datalake.save(last_id, header, body, now=start_time)

            done = control.read()
            with Timer() as t:
                remaining = [b for b in books if b[0] not in done]
                ingest(datalake, control, remaining, start_time + timedelta(days=1))
            stored = [p for p in datalake.root.rglob("*") if p.is_file() and "body" in p.name]
            results.add(name, n, "recovery_ms", 1000 * t.seconds)
            results.add(name, n, "recovery_duplicated_books", len(stored) - len(datalake.book_ids()))
            results.add(name, n, "recovery_lost_books", len(set(ids) - datalake.book_ids()))

    return results.save()


if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument("--books", type=int, nargs="+", default=[50, 100, 200])
    run(parser.parse_args().books)
