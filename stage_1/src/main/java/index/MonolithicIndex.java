package index;

import com.google.gson.Gson;
import com.google.gson.GsonBuilder;
import com.google.gson.reflect.TypeToken;

import java.io.IOException;
import java.lang.reflect.Type;
import java.nio.file.Files;
import java.nio.file.Path;
import java.util.ArrayList;
import java.util.Collections;
import java.util.HashMap;
import java.util.List;
import java.util.Map;

public class MonolithicIndex implements IndexStore {

    private final Path file;

    private final Gson gson =
            new GsonBuilder().setPrettyPrinting().create();

    private final Map<String, Map<Integer, List<Integer>>> index =
            new HashMap<>();

    public MonolithicIndex(Path root)
            throws IOException {

        Files.createDirectories(root);

        file =
                root.resolve(
                        "inverted_index.json"
                );

        load();
    }

    private void load()
            throws IOException {

        if (!Files.exists(file)
                || Files.size(file) == 0) {
            return;
        }

        String json =
                Files.readString(file);

        Type type =
                new TypeToken<
                        Map<String,
                                Map<String,
                                        List<Integer>>>
                        >() {
                        }.getType();

        Map<String, Map<String, List<Integer>>> raw =
                gson.fromJson(json, type);

        if (raw == null) {
            return;
        }

        for (
                Map.Entry<String, Map<String, List<Integer>>> entry
                        : raw.entrySet()
        ) {

            Map<Integer, List<Integer>> postings =
                    new HashMap<>();

            for (
                    Map.Entry<String, List<Integer>> doc
                            : entry.getValue().entrySet()
            ) {

                postings.put(
                        Integer.parseInt(doc.getKey()),
                        new ArrayList<>(doc.getValue())
                );
            }

            index.put(
                    entry.getKey(),
                    postings
            );
        }
    }

    @Override
    public void addBook(
            int bookId,
            Map<String, List<Integer>> tokens
    ) {

        for (
                Map.Entry<String, List<Integer>> entry
                        : tokens.entrySet()
        ) {

            index
                    .computeIfAbsent(
                            entry.getKey(),
                            k -> new HashMap<>()
                    )
                    .put(
                            bookId,
                            new ArrayList<>(entry.getValue())
                    );
        }
    }

    @Override
    public void flush()
            throws IOException {

        Map<String, Map<String, List<Integer>>> output =
                new HashMap<>();

        for (
                Map.Entry<String, Map<Integer, List<Integer>>> entry
                        : index.entrySet()
        ) {

            Map<String, List<Integer>> docs =
                    new HashMap<>();

            for (
                    Map.Entry<Integer, List<Integer>> doc
                            : entry.getValue().entrySet()
            ) {

                docs.put(
                        String.valueOf(doc.getKey()),
                        doc.getValue()
                );
            }

            output.put(
                    entry.getKey(),
                    docs
            );
        }

        Files.writeString(
                file,
                gson.toJson(output)
        );
    }

    @Override
    public Map<Integer, List<Integer>> lookup(
            String term
    ) {

        Map<Integer, List<Integer>> result =
                index.get(
                        term.toLowerCase()
                );

        if (result == null) {
            return Collections.emptyMap();
        }

        return result;
    }

    @Override
    public void close() {
    }

    @Override
    public long diskFiles()
            throws IOException {

        return Files.exists(file) ? 1 : 0;
    }

    @Override
    public long diskBytes()
            throws IOException {

        return Files.exists(file)
                ? Files.size(file)
                : 0;
    }
}