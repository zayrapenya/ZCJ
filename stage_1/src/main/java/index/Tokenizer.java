package index;

import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;
import java.util.Locale;
import java.util.Map;
import java.util.regex.Matcher;
import java.util.regex.Pattern;

public class Tokenizer {


    private static final Pattern WORD =
            Pattern.compile("[a-z]+");

    private Tokenizer() {
    }

    public static Map<String, List<Integer>> tokenize(
            String text
    ) {

        Map<String, List<Integer>> result =
                new HashMap<>();

        Matcher matcher =
                WORD.matcher(
                        text.toLowerCase(Locale.ROOT)
                );

        int position = 0;

        while (matcher.find()) {

            String word =
                    matcher.group();

            result
                    .computeIfAbsent(
                            word,
                            k -> new ArrayList<>()
                    )
                    .add(position);

            position++;
        }

        return result;
    }
}
