"""Embedded database: table postings(term, book_id, positions) in SQLite."""

import sqlite3
from pathlib import Path

from ..config import DATAMARTS_DIR
from .base import InvertedIndex, Postings, build_postings


class SQLiteIndex(InvertedIndex):
    name = "sqlite"

    def __init__(self, root: Path = DATAMARTS_DIR / "inverted_index.db"):
        super().__init__(root)
        self.root.parent.mkdir(parents=True, exist_ok=True)
        self.conn = sqlite3.connect(self.root)
        self.conn.execute(
            """CREATE TABLE IF NOT EXISTS postings (
                   term      TEXT    NOT NULL,
                   book_id   INTEGER NOT NULL,
                   positions TEXT    NOT NULL,
                   PRIMARY KEY (term, book_id)
               ) WITHOUT ROWID"""
        )

    def add_book(self, book_id, tokens):
        rows = ((term, book_id, ",".join(map(str, positions)))
                for term, positions in build_postings(book_id, tokens).items())
        self.conn.executemany("INSERT OR REPLACE INTO postings VALUES (?, ?, ?)", rows)
        self.conn.commit()

    def lookup(self, term) -> Postings:
        rows = self.conn.execute("SELECT book_id, positions FROM postings WHERE term = ?", (term,))
        return {book: [int(p) for p in positions.split(",")] for book, positions in rows}

    def close(self):
        self.conn.close()
