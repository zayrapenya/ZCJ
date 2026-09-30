package index;

import java.util.ArrayList;
import java.util.HashMap;
import java.util.HashSet;
import java.util.List;
import java.util.Map;
import java.util.Set;

public class SearchEngine {

    private final IndexStore index;

    public SearchEngine(IndexStore index) {
        this.index = index;
    }

    public Map<Integer, List<Integer>> search(
            String word
    ) throws Exception {

        return index.lookup(
                word.toLowerCase()
        );
    }

    public Map<Integer, List<Integer>> searchAnd(
            String word1,
            String word2
    ) throws Exception {

        Map<Integer, List<Integer>> first =
                search(word1);

        Map<Integer, List<Integer>> second =
                search(word2);

        Set<Integer> common =
                new HashSet<>(first.keySet());

        common.retainAll(
                second.keySet()
        );

        Map<Integer, List<Integer>> result =
                new HashMap<>();

        for (Integer bookId : common) {

            result.put(
                    bookId,
                    first.get(bookId)
            );
        }

        return result;
    }

    public Map<Integer, List<Integer>> searchPhrase(
            String phrase
    ) throws Exception {

        List<String> words =
                new ArrayList<>(
                        Tokenizer.tokenize(
                                phrase
                        ).keySet()
                );

        if (words.isEmpty()) {
            return new HashMap<>();
        }

        /*
         * Tokenizer.tokenize usa un HashMap, así que para una frase
         * necesitamos obtener los tokens respetando su orden.
         */
        words =
                orderedTokens(phrase);

        if (words.size() == 1) {
            return search(words.get(0));
        }

        Map<String, Map<Integer, List<Integer>>> postings =
                new HashMap<>();

        for (String word : words) {
            postings.put(
                    word,
                    search(word)
            );
        }

        Map<Integer, List<Integer>> first =
                postings.get(words.get(0));

        Map<Integer, List<Integer>> result =
                new HashMap<>();

        for (
                Map.Entry<Integer, List<Integer>> entry
                        : first.entrySet()
        ) {

            int bookId =
                    entry.getKey();

            for (
                    Integer start :
                    entry.getValue()
            ) {

                boolean matches = true;

                for (
                        int i = 1;
                        i < words.size();
                        i++
                ) {

                    List<Integer> positions =
                            postings
                                    .get(words.get(i))
                                    .get(bookId);

                    if (
                            positions == null
                                    || !positions.contains(
                                            start + i
                                    )
                    ) {

                        matches = false;
                        break;
                    }
                }

                if (matches) {

                    result
                            .computeIfAbsent(
                                    bookId,
                                    k -> new ArrayList<>()
                            )
                            .add(start);
                }
            }
        }

        return result;
    }

    private List<String> orderedTokens(
            String text
    ) {

        List<String> result =
                new ArrayList<>();

        String lower =
                text.toLowerCase();

        java.util.regex.Matcher matcher =
                java.util.regex.Pattern
                        .compile("[\\p{L}\\p{N}]+")
                        .matcher(lower);

        while (matcher.find()) {
            result.add(
                    matcher.group()
            );
        }

        return result;
    }
}