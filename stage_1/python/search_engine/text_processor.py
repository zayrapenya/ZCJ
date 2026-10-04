"""Tokenizer shared by the indexer and the search module.

It follows exactly the same rules as the Java version (TextProcessor.java) so that
both implementations produce equivalent indexes and can be benchmarked fairly:
lowercase the text, replace every non [a-z] character by a space and split.
"""

import re

_NON_LETTERS = re.compile(r"[^a-z]+")


def tokenize(text: str) -> list[str]:
    """Return the list of normalized words of ``text`` in order of appearance."""
    return _NON_LETTERS.sub(" ", text.lower()).split()
