# Stage 1 – Data layer (Python implementation)

Python version of the Stage 1 search engine data layer. It follows the same
preprocessing rules as the Java implementation (`stage_1/src`) so both produce
equivalent datalakes and inverted indexes and can be benchmarked against each other.

## Structure

```
stage_1/python/
├── main.py                    # command line entry point
├── requirements.txt
├── sample_data/               # 5 raw Gutenberg books to test the pipeline offline
├── search_engine/
│   ├── config.py              # paths and constants
│   ├── downloader.py          # download from Gutenberg + split header/body
│   ├── text_processor.py      # tokenizer (lowercase, [^a-z]+ -> space), same as Java
│   ├── datalake.py            # time / book / range based datalake layouts
│   ├── metadata.py            # header parsing (regex) + SQLite metadata datamart
│   ├── indexes/               # inverted index structures
│   │   ├── monolithic.py      #   datamarts/inverted_index.json
│   │   ├── hierarchical.py    #   datamarts/inverted_index/<LETTER>/<term>.txt
│   │   ├── sqlite_index.py    #   datamarts/inverted_index.db
│   │   └── mongo_index.py     #   MongoDB collection (optional)
│   ├── search.py              # word, AND and phrase queries
│   └── control.py             # control layer (control/*.txt) and pipeline
└── benchmarks/
    ├── bench_datalake.py
    ├── bench_index.py
    └── bench_metadata.py
```

Generated at runtime (ignored by git): `datalake/`, `datamarts/`, `control/`,
`bench_workspace/`, `benchmarks/results/`.

## Setup

Requires Python 3.10+.

```bash
cd stage_1/python
python -m venv .venv
.venv\Scripts\activate          # Windows  (Linux/macOS: source .venv/bin/activate)
pip install -r requirements.txt
```

MongoDB is optional. To include it: `pip install pymongo`, start a server on
`mongodb://localhost:27017` (or set `MONGO_URI`) and use `--index mongo`.

## Usage

Global options (before the command):
`--datalake {time,book,range}` (default `time`) and
`--index {monolithic,hierarchical,sqlite,mongo}` (default `monolithic`).

```bash
# Ingest the offline sample dataset (download -> datalake -> metadata + index)
python main.py sample

# Download specific books from Project Gutenberg and index them
python main.py download 1342 84 1661
python main.py index

# Let the control layer decide: index pending books or download new ones
python main.py run --steps 10                 # random ids
python main.py run --steps 6 --ids 11 98 2701

# Queries
python main.py search darcy
python main.py search "darcy love"            # AND
python main.py search "mr darcy" --phrase     # exact phrase
python main.py metadata --author Austen
python main.py metadata --id 1342

# Same pipeline with other structures
python main.py --datalake book --index sqlite sample
```

### Control layer

`control/downloaded_books.txt` and `control/indexed_books.txt` store the ids of the
books that finished each step (`failed_books.txt` stores ids that do not exist).
An id is written only after the step completes, so an interrupted run can simply be
restarted: pending books are detected as `downloaded - indexed` and re-indexing a book
overwrites its postings instead of duplicating them.

## Benchmarks

Run from `stage_1/python`. They use the sample dataset (cycled with synthetic ids to
reach the requested size), so they need no internet connection. Results are written to
`benchmarks/results/python_<benchmark>.csv` with the columns
`language,benchmark,structure,n_books,metric,value`, shared with the other languages.

```bash
python -m benchmarks.bench_datalake --books 50 100 200
python -m benchmarks.bench_index    --books 5 10 20
python -m benchmarks.bench_metadata --books 100 1000 10000
```

| Benchmark | Structures | Metrics |
|-----------|-----------|---------|
| datalake  | time, book, range | write throughput, lookup time, incremental detection, recovery (duplicated/lost books), files/dirs/bytes |
| index     | monolithic, hierarchical, sqlite, mongo | indexing time, peak RAM, query time (cold/avg), update one book, disk usage |
| metadata  | sqlite | insert rate, query by author / title / id |

Query workload used by the index benchmark (must be the same in every language):
words `darcy, monster, alice, holmes, love, the, revolution, creature, queen, zzzz`;
AND `darcy+love, monster+night, alice+queen, holmes+watson`;
phrases `mr darcy, sherlock holmes, the white rabbit, it was the best of times`.
