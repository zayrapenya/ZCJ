package datalake;

import java.util.ArrayList;
import java.util.HashMap;
import java.util.HashSet;
import java.util.List;
import java.util.Map;
import java.util.Set;

public class InvertedIndex {

    private final Map<String, Map<Integer, List<Integer>>> index;

    public InvertedIndex() {
        index = new HashMap<>();
    }

    public void addDocument(
            int documentId,
            List<String> words
    ) {

        for (int position = 0;
             position < words.size();
             position++) {

            String word = words.get(position);

            index
                .computeIfAbsent(
                        word,
                        k -> new HashMap<>()
                )
                .computeIfAbsent(
                        documentId,
                        k -> new ArrayList<>()
                )
                .add(position);
        }
    }

    public Map<Integer, List<Integer>> search(
            String word
    ) {

        return index.getOrDefault(
                word.toLowerCase(),
                new HashMap<>()
        );
    }

    public Map<Integer, List<Integer>> searchAnd(
        String word1,
        String word2
) {

    Map<Integer, List<Integer>> result =
            new HashMap<>();

    Map<Integer, List<Integer>> documents1 =
            search(word1);

    Map<Integer, List<Integer>> documents2 =
            search(word2);

    Set<Integer> commonDocuments =
            new HashSet<>(documents1.keySet());

    commonDocuments.retainAll(
            documents2.keySet()
    );

    for (Integer documentId : commonDocuments) {

        result.put(
                documentId,
                documents1.get(documentId)
        );
    }

    return result;
}

    public Map<Integer, List<Integer>> searchPhrase(
            String phrase
    ) {

        List<String> words =
                TextProcessor.tokenize(phrase);

        Map<Integer, List<Integer>> result =
                new HashMap<>();

        if (words.isEmpty()) {
            return result;
        }

        if (words.size() == 1) {
            return search(words.get(0));
        }

        Map<Integer, List<Integer>> firstWordDocuments =
                search(words.get(0));

        for (Integer documentId :
                firstWordDocuments.keySet()) {

            List<Integer> possiblePositions =
                    firstWordDocuments.get(documentId);

            for (Integer startPosition :
                    possiblePositions) {

                boolean matches = true;

                for (int i = 1;
                     i < words.size();
                     i++) {

                    Map<Integer, List<Integer>> nextWordDocuments =
                            search(words.get(i));

                    List<Integer> nextPositions =
                            nextWordDocuments.get(documentId);

                    if (nextPositions == null
                            || !nextPositions.contains(
                                    startPosition + i
                            )) {

                        matches = false;
                        break;
                    }
                }

                if (matches) {

                    result
                        .computeIfAbsent(
                                documentId,
                                k -> new ArrayList<>()
                        )
                        .add(startPosition);
                }
            }
        }

        return result;
    }

    public int size() {
        return index.size();
    }

    public Map<String, Map<Integer, List<Integer>>> getIndex() {
        return index;
    }
}