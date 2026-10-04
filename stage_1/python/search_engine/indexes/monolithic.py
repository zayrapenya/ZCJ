"""Single monolithic file: datamarts/inverted_index.json

{"adventure": {"5": [10, 250], "1342": [7]}, ...}
"""

import json
from pathlib import Path

from ..config import DATAMARTS_DIR
from .base import InvertedIndex, Postings, build_postings


class MonolithicIndex(InvertedIndex):
    name = "monolithic"

    def __init__(self, root: Path = DATAMARTS_DIR / "inverted_index.json"):
        super().__init__(root)
        self._data: dict[str, dict[str, list[int]]] | None = None
        self._dirty = False

    def _load(self) -> dict:
        if self._data is None:
            if self.root.exists():
                self._data = json.loads(self.root.read_text(encoding="utf-8"))
            else:
                self._data = {}
        return self._data

    def add_book(self, book_id, tokens):
        data = self._load()
        key = str(book_id)
        for term, positions in build_postings(book_id, tokens).items():
            data.setdefault(term, {})[key] = positions
        self._dirty = True

    def flush(self):
        # Every update requires rewriting the whole file
        if self._dirty:
            self.root.parent.mkdir(parents=True, exist_ok=True)
            self.root.write_text(json.dumps(self._data, separators=(",", ":")), encoding="utf-8")
            self._dirty = False

    def lookup(self, term) -> Postings:
        postings = self._load().get(term, {})
        return {int(book): positions for book, positions in postings.items()}
