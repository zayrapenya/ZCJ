"""Download books from Project Gutenberg and split them into header and body."""

import requests

from .config import END_MARKER, GUTENBERG_URL, START_MARKER


class InvalidBookError(Exception):
    """Raised when a downloaded text does not contain the Gutenberg markers."""


def download_book(book_id: int, timeout: float = 30) -> str | None:
    """Return the raw text of a book, or None if it does not exist."""
    response = requests.get(GUTENBERG_URL.format(id=book_id), timeout=timeout)
    if response.status_code == 404:
        return None
    response.raise_for_status()
    response.encoding = "utf-8"
    return response.text


def split_book(text: str) -> tuple[str, str]:
    """Split a raw Gutenberg text into (header, body). The footer is discarded."""
    start = text.find(START_MARKER)
    end = text.find(END_MARKER)
    if start == -1 or end == -1:
        raise InvalidBookError("Gutenberg start/end markers not found")

    header = text[:start].strip()
    # Skip the rest of the marker line ("... EBOOK <TITLE> ***")
    body_start = text.find("\n", start)
    body = text[body_start:end].strip()
    return header, body
