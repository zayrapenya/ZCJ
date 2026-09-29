"""Datalake storage structures.

Three layouts are implemented behind the same interface so they can be benchmarked:

* ``TimeDatalake``  -> datalake/YYYYMMDD/HH/<id>.body.txt   (layout recommended by the guide)
* ``BookDatalake``  -> datalake/books/<id>/body.txt
* ``RangeDatalake`` -> datalake/ranges/<start>-<end>/<id>.body.txt  (buckets of N ids)
"""

from datetime import datetime
from pathlib import Path

from .config import DATALAKE_DIR


class Datalake:
    """Common interface of every datalake layout."""

    name = "base"

    def __init__(self, root: Path = DATALAKE_DIR):
        self.root = Path(root)

    # --- to be implemented by each layout -------------------------------------
    def _book_dir(self, book_id: int, now: datetime) -> Path:
        raise NotImplementedError

    def _file_names(self, book_id: int) -> tuple[str, str]:
        return f"{book_id}.header.txt", f"{book_id}.body.txt"

    def locate(self, book_id: int) -> tuple[Path, Path] | None:
        raise NotImplementedError

    # --- shared logic ---------------------------------------------------------
    def save(self, book_id: int, header: str, body: str, now: datetime | None = None) -> tuple[Path, Path]:
        """Store header and body of a book and return their paths."""
        directory = self._book_dir(book_id, now or datetime.now())
        directory.mkdir(parents=True, exist_ok=True)
        header_name, body_name = self._file_names(book_id)
        header_path, body_path = directory / header_name, directory / body_name
        header_path.write_text(header, encoding="utf-8")
        body_path.write_text(body, encoding="utf-8")
        return header_path, body_path

    def read(self, book_id: int) -> tuple[str, str] | None:
        """Return (header, body) of a stored book or None if it is not in the datalake."""
        paths = self.locate(book_id)
        if paths is None:
            return None
        header_path, body_path = paths
        return header_path.read_text(encoding="utf-8"), body_path.read_text(encoding="utf-8")

    def book_ids(self) -> set[int]:
        """Scan the datalake and return the ids of every stored book."""
        return {int(p.name.split(".")[0]) for p in self.root.rglob("*.body.txt")}

    def storage_stats(self) -> dict:
        """Number of files, directories and bytes used by the layout."""
        files = dirs = size = 0
        if self.root.exists():
            for p in self.root.rglob("*"):
                if p.is_dir():
                    dirs += 1
                else:
                    files += 1
                    size += p.stat().st_size
        return {"files": files, "dirs": dirs, "bytes": size}


class TimeDatalake(Datalake):
    """Books grouped by download date and hour."""

    name = "time"

    def _book_dir(self, book_id, now):
        return self.root / now.strftime("%Y%m%d") / now.strftime("%H")

    def locate(self, book_id):
        # The download date is unknown, so every date/hour folder has to be scanned
        header_name, body_name = self._file_names(book_id)
        for body in self.root.glob(f"*/*/{body_name}"):
            return body.with_name(header_name), body
        return None


class BookDatalake(Datalake):
    """One directory per book."""

    name = "book"

    def _book_dir(self, book_id, now):
        return self.root / "books" / str(book_id)

    def _file_names(self, book_id):
        return "header.txt", "body.txt"

    def locate(self, book_id):
        directory = self.root / "books" / str(book_id)
        body = directory / "body.txt"
        return (directory / "header.txt", body) if body.exists() else None

    def book_ids(self):
        books = self.root / "books"
        return {int(p.name) for p in books.iterdir() if (p / "body.txt").exists()} if books.exists() else set()


class RangeDatalake(Datalake):
    """Books grouped in folders of ``bucket_size`` consecutive ids."""

    name = "range"

    def __init__(self, root: Path = DATALAKE_DIR, bucket_size: int = 1000):
        super().__init__(root)
        self.bucket_size = bucket_size

    def _range_dir(self, book_id):
        start = (book_id // self.bucket_size) * self.bucket_size
        return self.root / "ranges" / f"{start:06d}-{start + self.bucket_size - 1:06d}"

    def _book_dir(self, book_id, now):
        return self._range_dir(book_id)

    def locate(self, book_id):
        header_name, body_name = self._file_names(book_id)
        body = self._range_dir(book_id) / body_name
        return (body.with_name(header_name), body) if body.exists() else None


LAYOUTS = {cls.name: cls for cls in (TimeDatalake, BookDatalake, RangeDatalake)}


def get_datalake(name: str = "time", root: Path = DATALAKE_DIR) -> Datalake:
    return LAYOUTS[name](root)
