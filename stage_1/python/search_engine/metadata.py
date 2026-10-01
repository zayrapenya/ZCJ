"""Metadata datamart: parse the header of each book and store it in SQLite."""

import re
import sqlite3
from pathlib import Path

from .config import DATAMARTS_DIR

FIELDS = {
    "title": r"^Title:\s*(.+)$",
    "author": r"^Author:\s*(.+)$",
    "release_date": r"^Release [Dd]ate:\s*(.+?)(?:\s*\[.*\])?$",
    "language": r"^Language:\s*(.+)$",
}
_PATTERNS = {field: re.compile(regex, re.MULTILINE) for field, regex in FIELDS.items()}


def parse_header(header: str) -> dict:
    """Extract title, author, release date and language from a Gutenberg header."""
    metadata = {}
    for field, pattern in _PATTERNS.items():
        match = pattern.search(header)
        metadata[field] = match.group(1).strip() if match else None
    return metadata


class MetadataStore:
    """SQLite table ``books(book_id, title, author, release_date, language, path)``."""

    def __init__(self, db_path: Path = DATAMARTS_DIR / "metadata.db"):
        db_path = Path(db_path)
        db_path.parent.mkdir(parents=True, exist_ok=True)
        self.conn = sqlite3.connect(db_path)
        self.conn.execute(
            """CREATE TABLE IF NOT EXISTS books (
                   book_id      INTEGER PRIMARY KEY,
                   title        TEXT,
                   author       TEXT,
                   release_date TEXT,
                   language     TEXT,
                   path         TEXT
               )"""
        )
        self.conn.execute("CREATE INDEX IF NOT EXISTS idx_author ON books(author)")
        self.conn.execute("CREATE INDEX IF NOT EXISTS idx_title ON books(title)")
        self.conn.commit()

    def insert(self, book_id: int, metadata: dict, path: str | None = None, commit: bool = True):
        self.conn.execute(
            "INSERT OR REPLACE INTO books VALUES (?, ?, ?, ?, ?, ?)",
            (book_id, metadata.get("title"), metadata.get("author"),
             metadata.get("release_date"), metadata.get("language"), path),
        )
        if commit:
            self.conn.commit()

    def get(self, book_id: int) -> dict | None:
        row = self.conn.execute("SELECT * FROM books WHERE book_id = ?", (book_id,)).fetchone()
        return self._to_dict(row) if row else None

    def by_author(self, author: str) -> list[dict]:
        rows = self.conn.execute("SELECT * FROM books WHERE author LIKE ?", (f"%{author}%",))
        return [self._to_dict(r) for r in rows]

    def by_language(self, language: str) -> list[dict]:
        rows = self.conn.execute("SELECT * FROM books WHERE language = ?", (language,))
        return [self._to_dict(r) for r in rows]

    def path_by_title(self, title: str) -> str | None:
        row = self.conn.execute("SELECT path FROM books WHERE title = ?", (title,)).fetchone()
        return row[0] if row else None

    def count(self) -> int:
        return self.conn.execute("SELECT COUNT(*) FROM books").fetchone()[0]

    def close(self):
        self.conn.close()

    @staticmethod
    def _to_dict(row) -> dict:
        keys = ("book_id", "title", "author", "release_date", "language", "path")
        return dict(zip(keys, row))
