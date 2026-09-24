package datalake;

import java.util.List;
import java.util.Map;

public class SearchEngine {

    private final InvertedIndex index;

    public SearchEngine(InvertedIndex index) {
        this.index = index;
    }

    public Map<Integer, List<Integer>> search(
            String word
    ) {

        return index.search(word);
    }

    public Map<Integer, List<Integer>> searchAnd(
            String word1,
            String word2
    ) {

        return index.searchAnd(
                word1,
                word2
        );
    }

    public Map<Integer, List<Integer>> searchPhrase(
            String phrase
    ) {

        return index.searchPhrase(
                phrase
        );
    }
}