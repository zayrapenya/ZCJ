package index;

import com.google.gson.Gson;
import com.google.gson.reflect.TypeToken;

import java.io.IOException;
import java.nio.file.Files;
import java.nio.file.Path;
import java.lang.reflect.Type;
import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;
import java.util.Map;

public class MonolithicIndex implements IndexStore {

    private final Path file;

    private final Gson gson =
            new Gson();

    private final Map<String, Map<Integer, List<Integer>>> index =
            new HashMap<>();

    public MonolithicIndex(Path directory)
            throws IOException {

        Files.createDirectories(directory);

        this.file =
                directory.resolve(
                        "inverted_index.json"
                );

        load();
    }

    private void load()
            throws IOException {

        if (!Files.exists(file)) {
            return;
        }

        String json =
                Files.readString(file);

        if (json.isBlank()) {
            return;
        }

        Type type =
                new TypeToken<
                        Map<String, Map<Integer, List<Integer>>>
                        >() {
                        }.getType();

        Map<String, Map<Integer, List<Integer>>> loaded =
                gson.fromJson(
                        json,
                        type
                );

        if (loaded != null) {
            index.putAll(loaded);
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
                            new ArrayList<>(
                                    entry.getValue()
                            )
                    );
        }
    }

    @Override
    public void flush()
            throws IOException {

        Files.writeString(
                file,
                gson.toJson(index)
        );
    }

    @Override
    public Map<Integer, List<Integer>> lookup(
            String term
    ) {

        Map<Integer, List<Integer>> result =
                index.get(term.toLowerCase());

        if (result == null) {
            return new HashMap<>();
        }

        return new HashMap<>(result);
    }

    @Override
    public void close() {
        // No persistent connection/resources.
    }

    @Override
    public long diskFiles()
            throws IOException {

        return Files.exists(file) ? 1 : 0;
    }

    @Override
    public long diskBytes()
            throws IOException {

        if (!Files.exists(file)) {
            return 0;
        }

        return Files.size(file);
    }
}
