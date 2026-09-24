package datalake;

import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;
import java.util.Map;

public class InvertedIndex {

    private final Map<String, Map<Integer, List<Integer>>> index;

    public InvertedIndex() {
        index = new HashMap<>();
    }

    public void addDocument(int documentId, List<String> words) {

        for (int position = 0; position < words.size(); position++) {

            String word = words.get(position);

            index
                .computeIfAbsent(word, k -> new HashMap<>())
                .computeIfAbsent(documentId, k -> new ArrayList<>())
                .add(position);
        }
    }

    public Map<Integer, List<Integer>> search(String word) {

        return index.getOrDefault(
                word.toLowerCase(),
                new HashMap<>()
        );
    }

    public int size() {
        return index.size();
    }

    public Map<String, Map<Integer, List<Integer>>> getIndex() {
        return index;
    }
}