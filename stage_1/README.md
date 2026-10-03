# Search Engine Project — Stage 1

## Overview

This project implements the data layer of a search engine using books from **Project Gutenberg**.

The Java implementation includes:

* Datalake with three storage structures.
* SQLite metadata Datamart.
* Three inverted-index implementations.
* Search engine with word, AND and phrase queries.
* Control layer for downloading and indexing books.
* Benchmarks for Datalake, index and metadata performance.

---

## Requirements

* Java 17+
* Maven

Check the versions:

```bash
java -version
mvn -version
```

---

## Project Structure

```text
stage_1/
├── pom.xml
├── README.md
├── sample_data/
│   ├── pg11.txt
│   ├── pg84.txt
│   ├── pg98.txt
│   ├── pg1342.txt
│   └── pg1661.txt
│
└── src/main/java/
    ├── Main.java
    │
    ├── control/
    │   ├── ControlManager.java
    │   └── Pipeline.java
    │
    ├── datalake/
    │   ├── BookDownloader.java
    │   ├── BookDatalake.java
    │   ├── BookPaths.java
    │   ├── BookProcessor.java
    │   ├── Datalake.java
    │   ├── DateTimeDatalake.java
    │   └── RangeDatalake.java
    │
    ├── index/
    │   ├── HierarchicalIndex.java
    │   ├── IndexStore.java
    │   ├── MonolithicIndex.java
    │   ├── SQLiteIndex.java
    │   ├── SearchEngine.java
    │   └── Tokenizer.java
    │
    ├── metadata/
    │   └── MetadataStore.java
    │
    └── benchmark/
        ├── BenchmarkCommon.java
        ├── DatalakeBenchmark.java
        ├── IndexBenchmark.java
        └── MetadataBenchmark.java
```

---

## Data Source

Books are obtained from **Project Gutenberg**.

The included sample dataset contains:

```text
11, 84, 98, 1342, 1661
```

The files are located in:

```text
sample_data/
```

---

## Datalake

Three storage structures are implemented:

### Date-Time

```text
datalake/YYYYMMDD/HH/
```

Implemented by `DateTimeDatalake`.

### Book-Based

```text
datalake/books/<BOOK_ID>/
```

Implemented by `BookDatalake`.

### Range-Based

```text
datalake/ranges/<RANGE>/
```

Implemented by `RangeDatalake`.

---

## Metadata

Book metadata is stored in SQLite:

```text
datamarts/metadata.db
```

`MetadataStore` stores information such as:

* Book ID
* Title
* Author
* Release date
* Language
* Body path

---

## Inverted Index

Three implementations are available:

* **Monolithic JSON** — `MonolithicIndex`
* **Hierarchical files** — `HierarchicalIndex`
* **SQLite** — `SQLiteIndex`

The `SearchEngine` supports:

* Single-word search
* AND search
* Exact phrase search

---

## Control Layer

The control layer is implemented by:

```text
ControlManager
Pipeline
```

It maintains:

```text
control/
├── downloaded_books.txt
├── indexed_books.txt
└── failed_books.txt
```

The pipeline first indexes books that have been downloaded but not indexed. If there are no pending books, it attempts to download a new book.

Control files are updated only after successful operations.

---

## Compilation

From the `stage_1` directory:

```bash
mvn clean compile
```

All commands are run with `mvn exec:java` from the `stage_1` directory, so that Maven
adds the SQLite and Gson dependencies to the classpath.

---

## Running the Application

### Process sample dataset

```bash
mvn -q exec:java -Dexec.mainClass=Main -Dexec.args="sample"
```

### Download books

```bash
mvn -q exec:java -Dexec.mainClass=Main -Dexec.args="download 11 84 98 1342 1661"
```

### Index pending books

```bash
mvn -q exec:java -Dexec.mainClass=Main -Dexec.args="index"
```

### Run the control pipeline

```bash
mvn -q exec:java -Dexec.mainClass=Main -Dexec.args="run 10"
```

### Search

```bash
mvn -q exec:java -Dexec.mainClass=Main -Dexec.args="search darcy"
```

AND search:

```bash
mvn -q exec:java -Dexec.mainClass=Main -Dexec.args="search darcy love"
```

Phrase search:

```bash
mvn -q exec:java -Dexec.mainClass=Main -Dexec.args="phrase mr darcy"
```

### Metadata

```bash
mvn -q exec:java -Dexec.mainClass=Main -Dexec.args="metadata 1342"
```

---

## Selecting Storage Structures

The Datalake and index can be selected from the command line.

```bash
mvn -q exec:java -Dexec.mainClass=Main -Dexec.args="--datalake time --index monolithic sample"
```

Available Datalakes:

```text
time
book
range
```

Available indexes:

```text
monolithic
hierarchical
sqlite
```

---

## Benchmarks

The `benchmark` package contains four classes:

* `BenchmarkCommon.java` — common benchmark utilities.
* `DatalakeBenchmark.java` — compares the three Datalake structures.
* `IndexBenchmark.java` — compares the three inverted-index structures.
* `MetadataBenchmark.java` — evaluates metadata operations with different dataset sizes.

Benchmark results are generated as:

```text
java_datalake.csv
java_index.csv
java_metadata.csv
```

---

## Generated Data

The following directories and benchmark outputs are generated during execution and are not stored in Git:

```text
target/
datalake/
datamarts/
control/
java_datalake.csv
java_index.csv
java_metadata.csv
```

The `sample_data/` directory is tracked so that the project can be tested reproducibly.

---

## Quick Start

```bash
git clone https://github.com/zayrapenya/ZCJ.git
cd ZCJ/stage_1
mvn clean compile
mvn -q exec:java -Dexec.mainClass=Main -Dexec.args="sample"
mvn -q exec:java -Dexec.mainClass=Main -Dexec.args="search darcy"
```

---

## Repository

GitHub:

https://github.com/zayrapenya/ZCJ
