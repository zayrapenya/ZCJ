"""Global paths and constants shared by every module of the Python implementation."""

from pathlib import Path

# All generated data lives next to this package (stage_1/python/...)
BASE_DIR = Path(__file__).resolve().parent.parent

DATALAKE_DIR = BASE_DIR / "datalake"
DATAMARTS_DIR = BASE_DIR / "datamarts"
CONTROL_DIR = BASE_DIR / "control"
SAMPLE_DIR = BASE_DIR / "sample_data"

GUTENBERG_URL = "https://www.gutenberg.org/cache/epub/{id}/pg{id}.txt"

START_MARKER = "*** START OF THE PROJECT GUTENBERG EBOOK"
END_MARKER = "*** END OF THE PROJECT GUTENBERG EBOOK"

TOTAL_BOOKS = 70000
