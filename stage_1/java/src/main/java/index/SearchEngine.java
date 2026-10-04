package index;

import java.util.ArrayList;
import java.util.HashMap;
import java.util.HashSet;
import java.util.List;
import java.util.Locale;
import java.util.Map;
import java.util.Set;
import java.util.regex.Matcher;
import java.util.regex.Pattern;

public class SearchEngine {

    private final IndexStore index;

    private static final Pattern WORD_PATTERN =
            Pattern.compile("[a-z]+");

    public SearchEngine(IndexStore index) {
        this.index = index;
    }

    public Map<Integer, List<Integer>> search(
            String word
    ) throws Exception {

        String normalized =
                word.toLowerCase(Locale.ROOT);

        return index.lookup(normalized);
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
                orderedTokens(phrase);

        if (words.isEmpty()) {
            return new HashMap<>();
        }

        /*
         * First retrieve the postings of every word
         * in the phrase.
         */
        List<Map<Integer, List<Integer>>> postings =
                new ArrayList<>();

        for (String word : words) {

            Map<Integer, List<Integer>> result =
                    index.lookup(word);

            if (result.isEmpty()) {
                return new HashMap<>();
            }

            postings.add(result);
        }

        /*
         * A book can contain the phrase only if
         * it contains the first word.
         */
        Map<Integer, List<Integer>> first =
                postings.get(0);

        Map<Integer, List<Integer>> result =
                new HashMap<>();

        for (
                Map.Entry<Integer, List<Integer>> entry
                        : first.entrySet()
        ) {

            int bookId =
                    entry.getKey();


            List<Set<Integer>> positionSets =
                    new ArrayList<>();

            boolean bookContainsAllWords = true;

            for (int i = 0; i < postings.size(); i++) {

                List<Integer> positions =
                        postings
                                .get(i)
                                .get(bookId);

                if (positions == null) {
                    bookContainsAllWords = false;
                    break;
                }

                positionSets.add(
                        new HashSet<>(positions)
                );
            }

            if (!bookContainsAllWords) {
                continue;
            }


            for (
                    Integer start :
                    entry.getValue()
            ) {

                boolean matches = true;

                for (int i = 1; i < words.size(); i++) {

                    if (!positionSets
                            .get(i)
                            .contains(start + i)) {

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

        Matcher matcher =
                WORD_PATTERN.matcher(
                        text.toLowerCase(Locale.ROOT)
                );

        while (matcher.find()) {

            result.add(
                    matcher.group()
            );
        }

        return result;
    }
}
