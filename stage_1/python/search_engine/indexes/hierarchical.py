"""Hierarchical folder structure: one .txt file per term grouped by first letter.

datamarts/inverted_index/A/adventure.txt contains one line per book:
    5 10,250
    1342 7
"""

from pathlib import Path

from ..config import DATAMARTS_DIR
from .base import InvertedIndex, Postings, build_postings

# File names that Windows does not allow (tokens only contain letters)
_RESERVED = {"con", "prn", "aux", "nul"}


class HierarchicalIndex(InvertedIndex):
    name = "hierarchical"

    def __init__(self, root: Path = DATAMARTS_DIR / "inverted_index"):
        super().__init__(root)

    def _term_path(self, term: str) -> Path:
        file_name = f"{term}_" if term in _RESERVED else term
        return self.root / term[0].upper() / f"{file_name}.txt"

    def add_book(self, book_id, tokens):
        created_dirs = set()
        for term, positions in build_postings(book_id, tokens).items():
            path = self._term_path(term)
            if path.parent not in created_dirs:
                path.parent.mkdir(parents=True, exist_ok=True)
                created_dirs.add(path.parent)
            # Only the files of the terms of this book are touched
            with open(path, "a", encoding="utf-8") as f:
                f.write(f"{book_id} {','.join(map(str, positions))}\n")

    def lookup(self, term) -> Postings:
        path = self._term_path(term)
        if not path.exists():
            return {}
        postings = {}
        for line in path.read_text(encoding="utf-8").splitlines():
            book, positions = line.split(" ", 1)
            postings[int(book)] = [int(p) for p in positions.split(",")]
        return postings
