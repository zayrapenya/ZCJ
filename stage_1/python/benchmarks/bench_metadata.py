"""Metadata datamart benchmark (section 4.1 of the guide) on SQLite.

  * insertion speed    rows per second for n books
  * query performance  books by author, path by title, book by id
  * scalability        repeated for every value of --books (hundreds -> tens of thousands)

The headers of the sample dataset are parsed once and reused with synthetic ids/authors.

Run from stage_1/python:  python -m benchmarks.bench_metadata --books 100 1000 10000
"""

import argparse
import random

from search_engine.metadata import MetadataStore, parse_header

from .common import Results, Timer, fresh_workspace, load_sample_books

QUERIES = 200
N_AUTHORS = 500


def run(sizes: list[int]):
    results = Results("metadata")
    templates = [parse_header(header) for header, _ in load_sample_books()]

    for n in sizes:
        rows = []
        for i in range(n):
            meta = dict(templates[i % len(templates)])
            meta["author"] = f"{meta['author']} {i % N_AUTHORS}"
            meta["title"] = f"{meta['title']} #{i}"
            rows.append((i + 1, meta, f"datalake/books/{i + 1}/body.txt"))

        store = MetadataStore(fresh_workspace("metadata") / "metadata.db")
        with Timer() as t:
            for book_id, meta, path in rows:
                store.insert(book_id, meta, path, commit=False)
            store.conn.commit()
        results.add("sqlite", n, "insert_rows_per_sec", n / t.seconds)

        rng = random.Random(42)
        sample = [rng.choice(rows) for _ in range(QUERIES)]
        with Timer() as t:
            for _, meta, _ in sample:
                store.by_author(meta["author"])
        results.add("sqlite", n, "query_by_author_ms", 1000 * t.seconds / QUERIES)
        with Timer() as t:
            for _, meta, _ in sample:
                store.path_by_title(meta["title"])
        results.add("sqlite", n, "query_path_by_title_ms", 1000 * t.seconds / QUERIES)
        with Timer() as t:
            for book_id, _, _ in sample:
                store.get(book_id)
        results.add("sqlite", n, "query_by_id_ms", 1000 * t.seconds / QUERIES)
        store.close()

    return results.save()


if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument("--books", type=int, nargs="+", default=[100, 1000, 10000])
    run(parser.parse_args().books)
