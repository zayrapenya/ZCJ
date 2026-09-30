package index;

import com.google.gson.Gson;
import com.google.gson.GsonBuilder;
import com.google.gson.reflect.TypeToken;

import java.nio.file.*;
import java.util.*;

public class MonolithicIndex
        implements IndexStore {

    private final Path file;

    private final Map<String,
            Map<Integer, List<Integer>>> index =
            new HashMap<>();

    private final Gson gson =
            new GsonBuilder().setPrettyPrinting().create();

    public MonolithicIndex(Path file)
            throws Exception {

        this.file = file;

        if (Files.exists(file)) {

            String json =
                    Files.readString(file);

            Map<String,
                    Map<String, List<Double>>> raw =
                    gson.fromJson(
                        json,
                        new TypeToken<
                            Map<String,
                            Map<String, List<Double>>>
                        >() {}.getType()
                    );

            if (raw != null) {

                for (var termEntry :
                        raw.entrySet()) {

                    Map<Integer, List<Integer>> postings =
                            new HashMap<>();

                    for (var bookEntry :
                            termEntry.getValue().entrySet()) {

                        List<Integer> positions =
                                new ArrayList<>();

                        for (Double value :
                                bookEntry.getValue()) {

                            positions.add(
                                    value.intValue()
                            );
                        }

                        postings.put(
                                Integer.parseInt(
                                        bookEntry.getKey()
                                ),
                                positions
                        );
                    }

                    index.put(
                            termEntry.getKey(),
                            postings
                    );
                }
            }
        }
    }

    @Override
    public void addBook(
            int bookId,
            Map<String, List<Integer>> tokens) {

        for (var entry :
                tokens.entrySet()) {

            index.computeIfAbsent(
                    entry.getKey(),
                    k -> new HashMap<>()
            ).put(
                    bookId,
                    entry.getValue()
            );
        }
    }

    @Override
    public void flush()
            throws Exception {

        Files.createDirectories(
                file.getParent()
        );

        Files.writeString(
                file,
                gson.toJson(index)
        );
    }

    @Override
    public Map<Integer, List<Integer>> lookup(
            String term) {

        return index.getOrDefault(
                term.toLowerCase(),
                Collections.emptyMap()
        );
    }
}