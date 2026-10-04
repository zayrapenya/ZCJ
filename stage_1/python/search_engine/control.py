"""Minimal control layer: tracks the state of the pipeline with plain text control files.

control/
    downloaded_books.txt   books stored in the datalake (header + body)
    indexed_books.txt      books added to the inverted index and the metadata datamart
    failed_books.txt       ids that do not exist or have no Gutenberg markers (never retried)

An id is appended to a control file only *after* the corresponding step has finished,
so if the pipeline is interrupted the step is simply repeated on the next run
(no book is lost, and re-indexing a book overwrites its postings instead of duplicating them).
"""

import random
from pathlib import Path

from .config import CONTROL_DIR, SAMPLE_DIR, TOTAL_BOOKS
from .datalake import Datalake
from .downloader import InvalidBookError, download_book, split_book
from .indexes.base import InvertedIndex
from .metadata import MetadataStore, parse_header
from .text_processor import tokenize


class ControlFile:
    """Append-only set of book ids stored in a text file."""

    def __init__(self, path: Path):
        self.path = Path(path)

    def read(self) -> set[int]:
        if not self.path.exists():
            return set()
        return {int(line) for line in self.path.read_text(encoding="utf-8").split() if line.isdigit()}

    def add(self, book_id: int) -> None:
        self.path.parent.mkdir(parents=True, exist_ok=True)
        with open(self.path, "a", encoding="utf-8") as f:
            f.write(f"{book_id}\n")


def fetch_from_sample(book_id: int) -> str | None:
    """Offline source: read the raw book from sample_data/pg<ID>.txt."""
    path = SAMPLE_DIR / f"pg{book_id}.txt"
    return path.read_text(encoding="utf-8") if path.exists() else None


class Pipeline:
    def __init__(self, datalake: Datalake, index: InvertedIndex, metadata: MetadataStore,
                 control_dir: Path = CONTROL_DIR, fetch=download_book, verbose: bool = True):
        self.datalake = datalake
        self.index = index
        self.metadata = metadata
        self.fetch = fetch
        self.verbose = verbose
        self.downloaded = ControlFile(Path(control_dir) / "downloaded_books.txt")
        self.indexed = ControlFile(Path(control_dir) / "indexed_books.txt")
        self.failed = ControlFile(Path(control_dir) / "failed_books.txt")

    def log(self, message: str):
        if self.verbose:
            print(f"[CONTROL] {message}")

    # --- single operations ----------------------------------------------------
    def download(self, book_id: int) -> bool:
        if book_id in self.downloaded.read():
            self.log(f"Book {book_id} already downloaded, skipping.")
            return True
        try:
            text = self.fetch(book_id)
            if text is None:
                raise InvalidBookError("book not found")
            header, body = split_book(text)
        except InvalidBookError as e:
            self.log(f"Book {book_id} discarded ({e}).")
            self.failed.add(book_id)
            return False
        self.datalake.save(book_id, header, body)
        self.downloaded.add(book_id)
        self.log(f"Book {book_id} successfully downloaded.")
        return True

    def index_book(self, book_id: int) -> bool:
        stored = self.datalake.read(book_id)
        if stored is None:
            self.log(f"Book {book_id} not found in the datalake.")
            return False
        header, body = stored
        _, body_path = self.datalake.locate(book_id)
        self.metadata.insert(book_id, parse_header(header), str(body_path))
        self.index.add_book(book_id, tokenize(body))
        self.index.flush()
        self.indexed.add(book_id)
        self.log(f"Book {book_id} successfully indexed.")
        return True

    def pending(self) -> set[int]:
        return self.downloaded.read() - self.indexed.read()

    # --- orchestration (section 5.2 of the guide) ----------------------------
    def step(self, candidates: list[int] | None = None) -> None:
        """Index one pending book or, if there is none, download a new one."""
        ready_to_index = self.pending()
        if ready_to_index:
            book_id = min(ready_to_index)
            self.log(f"Scheduling book {book_id} for indexing...")
            self.index_book(book_id)
            return

        seen = self.downloaded.read() | self.failed.read()
        if candidates:
            new_ids = [b for b in candidates if b not in seen]
        else:  # retry up to 10 random ids to find a new book
            new_ids = [b for b in (random.randint(1, TOTAL_BOOKS) for _ in range(10)) if b not in seen]
        for book_id in new_ids:
            self.log(f"Downloading new book with ID {book_id}...")
            if self.download(book_id):
                return
        self.log("No new books to download.")

    def run(self, steps: int, candidates: list[int] | None = None) -> None:
        for _ in range(steps):
            self.step(candidates)
