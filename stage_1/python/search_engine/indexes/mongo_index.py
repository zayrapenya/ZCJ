"""NoSQL database: one MongoDB document per (term, book).

{"term": "adventure", "book_id": 5, "positions": [10, 250]}

Optional: requires ``pip install pymongo`` and a MongoDB server running
(default mongodb://localhost:27017, override with the MONGO_URI env variable).
"""

import os

from .base import InvertedIndex, Postings, build_postings


class MongoIndex(InvertedIndex):
    name = "mongo"

    def __init__(self, database: str = "search_engine", collection: str = "inverted_index"):
        from pymongo import ASCENDING, MongoClient

        super().__init__(root=".")
        self.client = MongoClient(os.environ.get("MONGO_URI", "mongodb://localhost:27017"),
                                  serverSelectionTimeoutMS=2000)
        self.client.admin.command("ping")  # fail fast if there is no server
        self.collection = self.client[database][collection]
        self.collection.create_index([("term", ASCENDING), ("book_id", ASCENDING)], unique=True)

    def add_book(self, book_id, tokens):
        from pymongo import ReplaceOne

        ops = [ReplaceOne({"term": term, "book_id": book_id},
                          {"term": term, "book_id": book_id, "positions": positions}, upsert=True)
               for term, positions in build_postings(book_id, tokens).items()]
        if ops:
            self.collection.bulk_write(ops, ordered=False)

    def lookup(self, term) -> Postings:
        return {doc["book_id"]: doc["positions"]
                for doc in self.collection.find({"term": term}, {"_id": 0, "book_id": 1, "positions": 1})}

    def drop(self):
        self.collection.drop()

    def disk_usage(self):
        stats = self.collection.database.command("collstats", self.collection.name)
        return {"files": 1, "bytes": stats.get("storageSize", 0) + stats.get("totalIndexSize", 0)}

    def close(self):
        self.client.close()
