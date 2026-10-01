package index;

import java.io.BufferedWriter;
import java.io.IOException;
import java.nio.file.Files;
import java.nio.file.Path;
import java.nio.file.StandardOpenOption;
import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;
import java.util.Map;
import java.util.stream.Stream;

public class HierarchicalIndex implements IndexStore {

    private final Path root;

    public HierarchicalIndex(Path directory)
            throws IOException {

        root =
                directory.resolve(
                        "inverted_index"
                );

        Files.createDirectories(root);
    }

    private Path pathFor(String term)
            throws IOException {

        String normalized =
                term.toLowerCase();

        if (normalized.isEmpty()) {
            throw new IllegalArgumentException(
                    "El término no puede estar vacío"
            );
        }

        String first =
                normalized.substring(0, 1).toUpperCase();

        Path directory =
                root.resolve(first);

        Files.createDirectories(directory);

        return directory.resolve(
                normalized + ".txt"
        );
    }

    @Override
    public void addBook(
            int bookId,
            Map<String, List<Integer>> tokens
    ) throws IOException {

        for (
                Map.Entry<String, List<Integer>> entry
                        : tokens.entrySet()
        ) {

            Path file =
                    pathFor(entry.getKey());

            StringBuilder positions =
                    new StringBuilder();

            for (
                    int i = 0;
                    i < entry.getValue().size();
                    i++
            ) {

                if (i > 0) {
                    positions.append(",");
                }

                positions.append(
                        entry.getValue().get(i)
                );
            }

            String line =
                    bookId
                            + " "
                            + positions
                            + "\n";

            Files.writeString(
                    file,
                    line,
                    StandardOpenOption.CREATE,
                    StandardOpenOption.APPEND
            );
        }
    }

    @Override
    public void flush() {

    }

    @Override
    public Map<Integer, List<Integer>> lookup(
            String term
    ) throws IOException {

        String normalized =
                term.toLowerCase();

        if (normalized.isEmpty()) {
            return new HashMap<>();
        }

        String first =
                normalized.substring(0, 1).toUpperCase();

        Path file =
                root
                        .resolve(first)
                        .resolve(
                                normalized + ".txt"
                        );

        Map<Integer, List<Integer>> result =
                new HashMap<>();

        if (!Files.exists(file)) {
            return result;
        }

        for (String line : Files.readAllLines(file)) {

            line = line.trim();

            if (line.isEmpty()) {
                continue;
            }

            String[] parts =
                    line.split(
                            "\\s+",
                            2
                    );

            if (parts.length != 2) {
                continue;
            }

            int bookId =
                    Integer.parseInt(parts[0]);

            List<Integer> positions =
                    new ArrayList<>();

            if (!parts[1].isBlank()) {

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

        try (Stream<Path> files =
                     Files.walk(root)) {

            return files
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

        try (Stream<Path> files =
                     Files.walk(root)) {

            return files
                    .filter(Files::isRegularFile)
                    .mapToLong(
                            path -> {
                                try {
                                    return Files.size(path);
                                } catch (IOException e) {
                                    return 0;
                                }
                            }
                    )
                    .sum();
        }
    }
}
