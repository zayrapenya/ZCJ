"""Queries over any inverted index: single word, AND and exact phrase (same as the Java SearchEngine)."""

from .indexes.base import InvertedIndex, Postings
from .text_processor import tokenize


class SearchEngine:
    def __init__(self, index: InvertedIndex):
        self.index = index

    def search(self, word: str) -> Postings:
        terms = tokenize(word)
        return self.index.lookup(terms[0]) if terms else {}

    def search_and(self, *words: str) -> Postings:
        """Books containing every word (positions of the first word are returned)."""
        results = [self.search(w) for w in words]
        if not results:
            return {}
        common = set(results[0]).intersection(*results[1:])
        return {book: results[0][book] for book in common}

    def search_phrase(self, phrase: str) -> Postings:
        """Books where the words appear consecutively; returns the start positions."""
        terms = tokenize(phrase)
        if not terms:
            return {}
        postings = [self.index.lookup(t) for t in terms]
        common = set(postings[0]).intersection(*postings[1:])
        result = {}
        for book in common:
            following = [set(p[book]) for p in postings[1:]]
            starts = [s for s in postings[0][book]
                      if all(s + i + 1 in positions for i, positions in enumerate(following))]
            if starts:
                result[book] = starts
        return result
