package index;

import java.io.IOException;
import java.nio.file.Files;
import java.nio.file.Path;
import java.nio.file.StandardOpenOption;
import java.util.ArrayList;
import java.util.Collections;
import java.util.HashMap;
import java.util.List;
import java.util.Map;
import java.util.stream.Stream;

public class HierarchicalIndex implements IndexStore {

    private final Path root;

    private final Map<String, Map<Integer, List<Integer>>> pending =
            new HashMap<>();

    public HierarchicalIndex(Path root)
            throws IOException {

        this.root = root;

        Files.createDirectories(root);
    }

    private String safeFileName(String term) {

        String lower =
                term.toLowerCase();

        if (
                lower.equals("con")
                        || lower.equals("prn")
                        || lower.equals("aux")
                        || lower.equals("nul")
        ) {
            return lower + "_";
        }

        return lower;
    }

    private Path pathFor(String term)
            throws IOException {

        String safe =
                safeFileName(term);

        String first =
                safe.substring(0, 1)
                        .toUpperCase();

        Path directory =
                root.resolve(first);

        Files.createDirectories(directory);

        return directory.resolve(
                safe + ".txt"
        );
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

            pending
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

        for (
                Map.Entry<String, Map<Integer, List<Integer>>> entry
                        : pending.entrySet()
        ) {

            Path file =
                    pathFor(entry.getKey());

            for (
                    Map.Entry<Integer, List<Integer>> doc
                            : entry.getValue().entrySet()
            ) {

                String positions =
                        doc.getValue()
                                .stream()
                                .map(String::valueOf)
                                .reduce(
                                        (a, b) -> a + "," + b
                                )
                                .orElse("");

                String line =
                        doc.getKey()
                                + " "
                                + positions
                                + System.lineSeparator();

                Files.writeString(
                        file,
                        line,
                        StandardOpenOption.CREATE,
                        StandardOpenOption.APPEND
                );
            }
        }

        pending.clear();
    }

    @Override
    public Map<Integer, List<Integer>> lookup(
            String term
    ) throws IOException {

        String safe =
                safeFileName(
                        term.toLowerCase()
                );

        Path file =
                pathFor(safe);

        if (!Files.exists(file)) {
            return Collections.emptyMap();
        }

        Map<Integer, List<Integer>> result =
                new HashMap<>();

        for (
                String line :
                Files.readAllLines(file)
        ) {

            line = line.trim();

            if (line.isEmpty()) {
                continue;
            }

            String[] parts =
                    line.split(
                            "\\s+",
                            2
                    );

            int bookId =
                    Integer.parseInt(parts[0]);

            List<Integer> positions =
                    new ArrayList<>();

            if (parts.length > 1
                    && !parts[1].isEmpty()) {

                for (
                        String position :
                        parts[1].split(",")
                ) {

                    positions.add(
                            Integer.parseInt(position)
                    );
                }
            }

            result.put(
                    bookId,
                    positions
            );
        }

        return result;
    }

    @Override
    public void close() {
    }

    @Override
    public long diskFiles()
            throws IOException {

        if (!Files.exists(root)) {
            return 0;
        }

        try (Stream<Path> stream =
                     Files.walk(root)) {

            return stream
                    .filter(Files::isRegularFile)
                    .count();
        }
    }

    @Override
    public long diskBytes()
            throws IOException {

        if (!Files.exists(root)) {
            return 0;
        }

        try (Stream<Path> stream =
                     Files.walk(root)) {

            return stream
                    .filter(Files::isRegularFile)
                    .mapToLong(path -> {
                        try {
                            return Files.size(path);
                        } catch (IOException e) {
                            return 0;
                        }
                    })
                    .sum();
        }
    }
}