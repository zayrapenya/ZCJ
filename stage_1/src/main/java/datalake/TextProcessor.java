package datalake;

import java.util.ArrayList;
import java.util.List;

public class TextProcessor {

    public static List<String> tokenize(String text) {

        List<String> tokens = new ArrayList<>();

        String normalizedText = text
                .toLowerCase()
                .replaceAll("[^a-zA-Z]+", " ");

        String[] words = normalizedText.trim().split("\\s+");

        for (String word : words) {

            if (!word.isEmpty()) {
                tokens.add(word);
            }
        }

        return tokens;
    }
}