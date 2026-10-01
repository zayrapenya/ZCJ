"""Command line entry point of the Python implementation.

Examples:
    python main.py download 1342 84 1661        # download into the datalake
    python main.py index                         # index every downloaded book not yet indexed
    python main.py run --steps 10                # control layer: alternate download / index
    python main.py sample                        # ingest the offline sample dataset
    python main.py search "mr darcy" --phrase
    python main.py metadata --author Austen
"""

import argparse

from search_engine.control import Pipeline, fetch_from_sample
from search_engine.config import SAMPLE_DIR
from search_engine.datalake import LAYOUTS, get_datalake
from search_engine.indexes import INDEX_TYPES, get_index
from search_engine.metadata import MetadataStore
from search_engine.search import SearchEngine


def build_pipeline(args, fetch=None) -> Pipeline:
    kwargs = {"fetch": fetch} if fetch else {}
    return Pipeline(get_datalake(args.datalake), get_index(args.index), MetadataStore(), **kwargs)


def main():
    parser = argparse.ArgumentParser(description="Stage 1 data layer (Python)")
    parser.add_argument("--datalake", choices=list(LAYOUTS), default="time", help="datalake layout")
    parser.add_argument("--index", choices=INDEX_TYPES, default="monolithic", help="inverted index structure")
    sub = parser.add_subparsers(dest="command", required=True)

    p = sub.add_parser("download", help="download books by id")
    p.add_argument("ids", type=int, nargs="+")

    sub.add_parser("index", help="index every pending book")

    p = sub.add_parser("run", help="run the control layer N steps")
    p.add_argument("--steps", type=int, default=10)
    p.add_argument("--ids", type=int, nargs="*", help="candidate ids (random if omitted)")

    sub.add_parser("sample", help="ingest the offline dataset in sample_data/")

    p = sub.add_parser("search", help="search the inverted index")
    p.add_argument("query")
    p.add_argument("--phrase", action="store_true", help="exact phrase instead of AND of words")

    p = sub.add_parser("metadata", help="query the metadata datamart")
    p.add_argument("--author")
    p.add_argument("--language")
    p.add_argument("--id", type=int)

    args = parser.parse_args()

    if args.command == "download":
        pipeline = build_pipeline(args)
        for book_id in args.ids:
            pipeline.download(book_id)

    elif args.command == "index":
        pipeline = build_pipeline(args)
        for book_id in sorted(pipeline.pending()):
            pipeline.index_book(book_id)
        pipeline.index.close()

    elif args.command == "run":
        pipeline = build_pipeline(args)
        pipeline.run(args.steps, args.ids)
        pipeline.index.close()

    elif args.command == "sample":
        ids = sorted(int(p.stem[2:]) for p in SAMPLE_DIR.glob("pg*.txt"))
        pipeline = build_pipeline(args, fetch=fetch_from_sample)
        pipeline.run(2 * len(ids), ids)
        pipeline.index.close()

    elif args.command == "search":
        engine = SearchEngine(get_index(args.index))
        words = args.query.split()
        results = engine.search_phrase(args.query) if args.phrase else engine.search_and(*words)
        metadata = MetadataStore()
        if not results:
            print("No results.")
        for book_id, positions in sorted(results.items(), key=lambda r: -len(r[1])):
            info = metadata.get(book_id) or {}
            print(f"{book_id:>6}  {len(positions):>5} hits  {info.get('title')} - {info.get('author')}")

    elif args.command == "metadata":
        store = MetadataStore()
        if args.id:
            rows = [store.get(args.id)]
        elif args.author:
            rows = store.by_author(args.author)
        elif args.language:
            rows = store.by_language(args.language)
        else:
            rows = [store.get(i) for (i,) in store.conn.execute("SELECT book_id FROM books ORDER BY book_id")]
        for row in filter(None, rows):
            print(row)


if __name__ == "__main__":
    main()
