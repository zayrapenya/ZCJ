"""Common interface of every inverted index structure."""

from collections import defaultdict
from pathlib import Path

Postings = dict[int, list[int]]  # book_id -> positions of the term inside the book


def build_postings(book_id: int, tokens: list[str]) -> dict[str, list[int]]:
    """Group the positions of every term of one book: term -> [positions]."""
    positions = defaultdict(list)
    for position, term in enumerate(tokens):
        positions[term].append(position)
    return positions


class InvertedIndex:
    name = "base"

    def __init__(self, root: Path):
        self.root = Path(root)

    def add_book(self, book_id: int, tokens: list[str]) -> None:
        """Add (or update) a book in the index without rebuilding it."""
        raise NotImplementedError

    def flush(self) -> None:
        """Persist pending changes (no-op for structures that write immediately)."""

    def lookup(self, term: str) -> Postings:
        raise NotImplementedError

    def close(self) -> None:
        self.flush()

    def disk_usage(self) -> dict:
        files = size = 0
        target = self.root
        if target.is_file():
            return {"files": 1, "bytes": target.stat().st_size}
        if target.exists():
            for p in target.rglob("*"):
                if p.is_file():
                    files += 1
                    size += p.stat().st_size
        return {"files": files, "bytes": size}
