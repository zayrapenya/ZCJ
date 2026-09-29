"""Inverted index structures that can be selected by name."""

from pathlib import Path

from ..config import DATAMARTS_DIR
from .base import InvertedIndex
from .hierarchical import HierarchicalIndex
from .monolithic import MonolithicIndex
from .sqlite_index import SQLiteIndex

INDEX_TYPES = ["monolithic", "hierarchical", "sqlite", "mongo"]


def get_index(name: str, datamarts: Path = DATAMARTS_DIR) -> InvertedIndex:
    datamarts = Path(datamarts)
    if name == "monolithic":
        return MonolithicIndex(datamarts / "inverted_index.json")
    if name == "hierarchical":
        return HierarchicalIndex(datamarts / "inverted_index")
    if name == "sqlite":
        return SQLiteIndex(datamarts / "inverted_index.db")
    if name == "mongo":
        from .mongo_index import MongoIndex
        return MongoIndex()
    raise ValueError(f"Unknown index type: {name}")
