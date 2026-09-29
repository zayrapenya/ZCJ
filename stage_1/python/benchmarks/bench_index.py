"""Inverted index benchmark (section 4.2 of the guide).

For each structure (monolithic, hierarchical, sqlite and mongo if available) it measures:
  * indexing speed     time to build the index from n books
  * query performance  average time of a fixed workload of words, AND and phrase queries
  * update cost        time to add one new book to an existing index
  * memory usage       peak RAM while indexing (tracemalloc)
  * disk usage         files and bytes
  * scalability        repeated for every value of --books

Run from stage_1/python:  python -m benchmarks.bench_index --books 5 10 20
"""

import argparse
import tracemalloc

from search_engine.indexes import get_index
from search_engine.search import SearchEngine
from search_engine.text_processor import tokenize

from .common import Results, Timer, fresh_workspace, synthetic_books

# Same query workload must be used by every language
WORDS = ["darcy", "monster", "alice", "holmes", "love", "the", "revolution", "creature", "queen", "zzzz"]
AND_QUERIES = [("darcy", "love"), ("monster", "night"), ("alice", "queen"), ("holmes", "watson")]
PHRASES = ["mr darcy", "sherlock holmes", "the white rabbit", "it was the best of times"]
REPEAT = 5


def available_structures(requested):
    structures = []
    for name in requested:
        if name == "mongo":
            try:
                get_index("mongo").drop()
            except Exception as e:  # pymongo not installed or no server running
                print(f"  (skipping mongo: {type(e).__name__})")
                continue
        structures.append(name)
    return structures


def run_queries(engine: SearchEngine):
    for word in WORDS:
        engine.search(word)
    for words in AND_QUERIES:
        engine.search_and(*words)
    for phrase in PHRASES:
        engine.search_phrase(phrase)


def run(sizes: list[int], structures: list[str]):
    results = Results("index")
    structures = available_structures(structures)
    n_queries = len(WORDS) + len(AND_QUERIES) + len(PHRASES)

    for n in sizes:
        books = synthetic_books(n + 1)
        tokenized = [(book_id, tokenize(body)) for book_id, _, body in books]
        initial, new_book = tokenized[:n], tokenized[n]

        for name in structures:
            workspace = fresh_workspace(f"index_{name}")
            index = get_index(name, workspace)
            if name == "mongo":
                index.drop()

            tracemalloc.start()
            with Timer() as t:
                for book_id, tokens in initial:
                    index.add_book(book_id, tokens)
                index.flush()
            peak = tracemalloc.get_traced_memory()[1]
            tracemalloc.stop()
            results.add(name, n, "indexing_sec", t.seconds)
            results.add(name, n, "indexing_peak_ram_mb", peak / 1e6)

            # Reopen the index so queries read from storage, not from the build process
            index.close()
            index = get_index(name, workspace)
            engine = SearchEngine(index)
            with Timer() as t:
                run_queries(engine)  # first run: cold (includes loading the index)
            results.add(name, n, "query_cold_total_ms", 1000 * t.seconds)
            with Timer() as t:
                for _ in range(REPEAT):
                    run_queries(engine)
            results.add(name, n, "query_avg_ms", 1000 * t.seconds / (REPEAT * n_queries))

            with Timer() as t:
                index.add_book(*new_book)
                index.flush()
            results.add(name, n, "update_one_book_sec", t.seconds)

            usage = index.disk_usage()
            results.add(name, n, "disk_files", usage["files"])
            results.add(name, n, "disk_mb", usage["bytes"] / 1e6)
            index.close()

    return results.save()


if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument("--books", type=int, nargs="+", default=[5, 10, 20])
    parser.add_argument("--structures", nargs="+", default=["monolithic", "hierarchical", "sqlite", "mongo"])
    args = parser.parse_args()
    run(args.books, args.structures)
