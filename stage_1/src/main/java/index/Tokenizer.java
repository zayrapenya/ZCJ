package index;

import java.util.*;
import java.util.regex.Matcher;
import java.util.regex.Pattern;

public class Tokenizer {

    private static final Pattern WORD =
            Pattern.compile("[\\p{L}\\p{N}]+");

    public static Map<String, List<Integer>> tokenize(
            String text) {

        Map<String, List<Integer>> result =
                new HashMap<>();

        Matcher matcher =
                WORD.matcher(text.toLowerCase());

        int position = 0;

        while (matcher.find()) {

            String word =
                    matcher.group();

            result.computeIfAbsent(
                    word,
                    k -> new ArrayList<>()
            ).add(position);

            position++;
        }

        return result;
    }
}