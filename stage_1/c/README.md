# Stage 1 – Data layer (C implementation)

C version of the Stage 1 search engine data layer. It follows exactly the same
preprocessing rules, file formats and query semantics as the Python
implementation (`stage_1/python`), so all languages produce equivalent datalakes
and inverted indexes and can be benchmarked against each other.

## Structure

```
stage_1/c/
├── Makefile
└── src/
    ├── main.c           # command line entry point
    ├── config.h         # paths and constants
    ├── util.c           # files, folders, high resolution timer
    ├── book.c           # header/body split (Gutenberg markers)
    ├── tokenizer.c      # tokenizer (lowercase, [^a-z]+ -> separator), same as Python/Java
    ├── hashmap.c        # hash table (open addressing, FNV-1a)
    ├── index.c          # in-memory inverted index (term -> book -> positions)
    ├── datalake.c       # time / book / range based datalake layouts
    ├── metadata.c       # header parsing + SQLite metadata datamart
    ├── disk_index.c     # inverted index structures on disk:
    │                    #   monolithic   -> datamarts/inverted_index.json
    │                    #   hierarchical -> datamarts/inverted_index/<LETTER>/<term>.txt
    │                    #   sqlite       -> datamarts/inverted_index.db
    ├── search.c         # word, AND and phrase queries
    ├── downloader.c     # download from Project Gutenberg (libcurl)
    ├── control.c        # control layer (control/*.txt) and pipeline
    ├── benchmark.c      # datalake, index and metadata benchmarks
    └── memtrack.c       # allocation counter used by the memory benchmark
```

The sample dataset is read from the shared folder `stage_1/sample_data/`.

Generated at runtime (ignored by git): `build/`, `datalake/`, `datamarts/`,
`control/`, `benchmarks/results/`.

## Setup

Requires a C11 compiler (`gcc`), `make`, and the libraries **SQLite 3** and
**libcurl**. Run every command **one by one**.

### Windows (MSYS2)

1. Install [MSYS2](https://www.msys2.org/).
2. Open the **MSYS2 UCRT64** terminal (not "MSYS" or "MINGW64") and install the
   compiler and the libraries:

   ```bash
   pacman -S --needed mingw-w64-ucrt-x86_64-gcc make mingw-w64-ucrt-x86_64-sqlite3 mingw-w64-ucrt-x86_64-curl
   ```

3. Go to this folder. In MSYS2, `C:\` is written `/c/`, and paths with spaces
   must be quoted:

   ```bash
   cd "/c/path/to/repo/stage_1/c"
   ```

> To paste in the MSYS2 window use **Shift + Insert** or right click → Paste.

### Linux (Debian / Ubuntu)

```bash
sudo apt install build-essential libsqlite3-dev libcurl4-openssl-dev
cd stage_1/c
```

### macOS

```bash
xcode-select --install        # compiler and make
cd stage_1/c                  # SQLite and libcurl are included in macOS
```

### Build

```bash
make
```

The program is created at `build/search_engine` (`build/search_engine.exe` on
Windows). `make clean` removes the compiled files.

### Quick check

```bash
./build/search_engine sample
./build/search_engine search "mr darcy" --phrase
```

The last command should list *Pride and Prejudice* with **277 hits**.

## Usage

```
./build/search_engine [--datalake time|book|range] [--index monolithic|hierarchical|sqlite] <command>
```

The datalake and index structures are chosen independently. Defaults:
`--datalake time --index monolithic` (same as Python). Use the same options in
every command that works on the same data.

| Command | Description |
|---------|-------------|
| `sample` | Ingest the 5 books of `stage_1/sample_data` (offline) |
| `download <id> [<id> ...]` | Download books from Project Gutenberg into the datalake |
| `index` | Index every downloaded book that is not indexed yet |
| `run [--steps N] [--ids <id> ...]` | Control layer: each step indexes a pending book or downloads a new one (random ids if `--ids` is not given) |
| `search "<words>"` | Books containing every word (AND) |
| `search "<phrase>" --phrase` | Exact phrase search |
| `metadata` | List the metadata of every book |
| `metadata --author X` / `--language X` / `--id N` | Metadata queries |
| `bench datalake\|index\|metadata\|all [sizes...]` | Run the benchmarks |

### Examples

```bash
./build/search_engine sample
./build/search_engine search holmes watson
./build/search_engine search "it was the best of times" --phrase
./build/search_engine metadata --author Austen

./build/search_engine download 2701          # Moby Dick (needs internet)
./build/search_engine index
./build/search_engine search whale

./build/search_engine --datalake range --index sqlite sample
./build/search_engine --datalake range --index sqlite search darcy
```

To start again from scratch, delete the generated data:

```bash
rm -rf datalake datamarts control
```

## Control layer

The state of the pipeline is stored in:

| File | Content |
|------|---------|
| `control/downloaded_books.txt` | Books stored in the datalake |
| `control/indexed_books.txt` | Books added to the index and the metadata datamart |
| `control/failed_books.txt` | Ids that do not exist or have no Gutenberg markers |

An id is written only after its step has finished, so if the program is
interrupted it can simply be run again: no book is lost or processed twice. A
network error does not mark a book as failed, so it is retried later.

## Benchmarks

```bash
./build/search_engine bench datalake      # 50, 100 and 200 books
./build/search_engine bench index         # 5, 10 and 20 books
./build/search_engine bench metadata      # 100, 1000 and 10000 books
./build/search_engine bench index 5       # custom sizes
```

They use the same sizes, synthetic books (ids from 100000, cycling over the
sample), query workload and metric names as the Python benchmarks. Scratch data
is written to the system temp folder (override it with the `BENCH_WORKSPACE`
environment variable) and the results are saved to:

```
benchmarks/results/c_datalake.csv
benchmarks/results/c_index.csv
benchmarks/results/c_metadata.csv
```

with the common format `language,benchmark,structure,n_books,metric,value`.
| Benchmark | Structures | Metrics |
|-----------|-----------|---------|
| datalake  | time, book, range | write throughput, lookup time, incremental detection, recovery (duplicated/lost books), files/dirs/bytes |
| index     | monolithic, hierarchical, sqlite | indexing time, peak RAM, query time (cold/avg), update one book, disk usage |
| metadata  | sqlite | insert rate, query by author / title / id |

Query workload used by the index benchmark (must be the same in every language):
words `darcy, monster, alice, holmes, love, the, revolution, creature, queen, zzzz`;
AND `darcy+love, monster+night, alice+queen, holmes+watson`;
phrases `mr darcy, sherlock holmes, the white rabbit, it was the best of times`.

Peak memory is measured by counting every `malloc`/`free` of the program
(`memtrack.c`), which is the equivalent of Python's `tracemalloc`.

> **Windows note:** the hierarchical index writes thousands of small files, which
> is very slow on Windows (several minutes for `bench index`). Let it finish and
> run the benchmarks one command at a time.

## Verification against Python

With the sample dataset the C implementation gives exactly the same results as
the Python version:

| Check | Value |
|-------|-------|
| Tokens per book (11, 84, 98, 1342, 1661) | 27,427 / 75,328 / 138,490 / 128,565 / 105,863 |
| Distinct terms in the index | 16,804 |
| `darcy` in book 1342 | 432 |
| Phrase `mr darcy` in book 1342 | 277 |
| Phrase `it was the best of times` | 1 (book 98) |

The hierarchical index files are byte-for-byte identical to the Python ones, and
the JSON file and the SQLite tables have the same content.