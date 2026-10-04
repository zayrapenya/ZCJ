# Search Engine Project — Stage 1: Building the Data Layer

**Big Data · Degree in Data Science and Engineering · Universidad de Las Palmas de Gran Canaria · 2026–2027**

**Group ZCJ:** Claudia Álvarez González · Zayra Penya Juan · Javier Fernández Muñoz

This repository contains the data layer of a search engine built from scratch with books from
[Project Gutenberg](https://www.gutenberg.org/). The same pipeline is implemented in **three
languages — Python, Java and C** — with the same dataset, tokenizer, file formats and query
workload, so that both the storage structures and the languages can be benchmarked against
each other.

## Pipeline

```text
Project Gutenberg ──download──> Datalake (header + body)
                                   │
                     ┌─────────────┴─────────────┐
                     ▼                           ▼
            Metadata datamart            Inverted index datamart
               (SQLite)               (JSON / hierarchical files / SQLite)
                     └─────────────┬─────────────┘
                                   ▼
                     Queries: word, AND, exact phrase

Control layer (control/*.txt): decides at each step whether to index a pending
book or download a new one, so no book is duplicated or lost.
```

Each implementation includes:

- **Datalake** with three layouts: time-based (`YYYYMMDD/HH/`), book-based (`books/<ID>/`)
  and range-based (`ranges/001000-001999/`).
- **Metadata datamart** in SQLite: title, author, release date, language and path.
- **Inverted index** with three structures: monolithic JSON, hierarchical folders (one file
  per term) and SQLite, storing the positions of every term to support phrase search.
- **Control layer** with `downloaded_books.txt`, `indexed_books.txt` and `failed_books.txt`.
- **Benchmarks** for the datalake, the inverted index and the metadata store, written as CSV
  with the common format `language,benchmark,structure,n_books,metric,value`.

## Repository structure

```text
stage_1/
├── sample_data/        # 5 Gutenberg books (11, 84, 98, 1342, 1661) for offline tests
├── java/               # Java implementation   → stage_1/java/README.md
├── python/             # Python implementation → stage_1/python/README.md
└── c/                  # C implementation      → stage_1/c/README.md
```

Generated data (`datalake/`, `datamarts/`, `control/`, benchmark results, `target/`,
`build/`) is not stored in Git.

## Quick start

Detailed setup and all the commands are in the README of each implementation.

| Language | Documentation | Ingest the sample dataset and search |
|----------|---------------|--------------------------------------|
| Python 3.10+ | [stage_1/python/README.md](stage_1/python/README.md) | `cd stage_1/python` → `pip install -r requirements.txt` → `python main.py sample` → `python main.py search "mr darcy" --phrase` |
| Java 17+ (Maven) | [stage_1/java/README.md](stage_1/java/README.md) | `cd stage_1/java` → `mvn clean compile` → `mvn -q exec:java -Dexec.mainClass=Main -Dexec.args="sample"` → `mvn -q exec:java -Dexec.mainClass=Main -Dexec.args="phrase mr darcy"` |
| C11 (gcc, SQLite, libcurl) | [stage_1/c/README.md](stage_1/c/README.md) | `cd stage_1/c` → `make` → `./build/search_engine sample` → `./build/search_engine search "mr darcy" --phrase` |

The three implementations return the same result: the phrase *mr darcy* appears 277 times in
*Pride and Prejudice* (book 1342).

## Main results

- The three implementations produce **identical outputs**: the same datalake files, the same
  16,804 terms in the inverted index and the same query results.
- **Datalake:** the **range-based** layout is recommended: direct lookup, no duplicated books
  after an interruption and very few directories. The time-based layout must scan every
  folder to locate a book and may duplicate a book after an interrupted ingestion.
- **Inverted index:** **SQLite** is recommended: constant memory usage and cheap incremental
  updates. The monolithic file is the fastest to query but must be kept in memory and
  rewritten on every update; the hierarchical structure is slowed down by thousands of small
  files.
- **Languages:** the ranking of the structures is the same in the three languages. C is the
  fastest and uses the least memory in the operations done in memory, Java is in between and
  Python is the slowest but needs the least code; operations dominated by the disk or by
  SQLite take a similar time in all of them.

The complete analysis is in the project report.
