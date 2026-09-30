package index;

import java.util.*;

public class SearchEngine {

    private final IndexStore store;

    public SearchEngine(IndexStore store) {
        this.store = store;
    }

    public Map<Integer, List<Integer>> word(
            String term)
            throws Exception {

        return store.lookup(
                term.toLowerCase()
        );
    }

    public Set<Integer> and(
            String... terms)
            throws Exception {

        if (terms.length == 0) {
            return Collections.emptySet();
        }

        Set<Integer> result =
                new HashSet<>(
                        store.lookup(
                                terms[0]
                        ).keySet()
                );

        for (int i = 1;
             i < terms.length;
             i++) {

            result.retainAll(
                    store.lookup(
                            terms[i]
                    ).keySet()
            );
        }

        return result;
    }

    public Set<Integer> phrase(
            String phrase)
            throws Exception {

        String[] words =
                phrase.toLowerCase()
                      .trim()
                      .split("\\s+");

        if (words.length == 0) {
            return Collections.emptySet();
        }

        Map<Integer, List<Integer>> first =
                store.lookup(words[0]);

        Set<Integer> result =
                new HashSet<>();

        for (Integer bookId :
                first.keySet()) {

            List<Integer> positions =
                    first.get(bookId);

            for (Integer start :
                    positions) {

                boolean match = true;

                for (int i = 1;
                     i < words.length;
                     i++) {

                    Map<Integer,
                            List<Integer>> next =
                            store.lookup(words[i]);

                    List<Integer> nextPositions =
                            next.get(bookId);

                    if (nextPositions == null
                            || !nextPositions.contains(
                                    start + i
                            )) {

                        match = false;
                        break;
                    }
                }

                if (match) {
                    result.add(bookId);
                    break;
                }
            }
        }

        return result;
    }
}