# Search Engine Project — Stage 1

## Overview

This project implements the data layer of a search engine using books from Project Gutenberg.

The system is divided into three main components:

- Datalake
- Datamarts
- Control layer

The project also includes benchmarks for comparing different storage and indexing structures.

The implementation is written in Java 17 and uses Maven.

---

## Architecture

```text
Project Gutenberg
       |
       v
BookDownloader
       |
       v
BookProcessor
       |
       v
Datalake
  |    |    |
  v    v    v
Date  Book Range
       |
       v
Datamarts
  |          |
  v          v
Metadata   Inverted Index
SQLite     |    |    |
          JSON  Hierarchical  SQLite
                |
                v
           SearchEngine

Control Layer
     |
     +-- downloaded_books.txt
     +-- indexed_books.txt
     +-- failed_books.txt


q
