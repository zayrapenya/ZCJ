package index;

import java.io.IOException;
import java.nio.file.*;
import java.util.*;

public class HierarchicalIndex
        implements IndexStore {

    private final Path root;

    private final Map<String,
            Map<Integer, List<Integer>>> pending =
            new HashMap<>();

    public HierarchicalIndex(Path root) {

        this.root = root;
    }

    private String safeFileName(String term) {

        String lower = term.toLowerCase();

        if (lower.equals("con")
                || lower.equals("prn")
                || lower.equals("aux")
                || lower.equals("nul")) {

            return lower + "_";
        }

        return lower;
    }

    private Path pathFor(String term) {

        String safe =
                safeFileName(term);

        String first =
                safe.substring(0, 1).toUpperCase();

        return root.resolve(first)
                .resolve(safe + ".txt");
    }

    @Override
    public void addBook(
            int bookId,
            Map<String, List<Integer>> tokens) {

        for (var entry :
                tokens.entrySet()) {

            pending.computeIfAbsent(
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
            throws IOException {

        for (var entry :
                pending.entrySet()) {

            String term =
                    entry.getKey();

            Path file =
                    pathFor(term);

            Files.createDirectories(
                    file.getParent()
            );

            for (var book :
                    entry.getValue().entrySet()) {

                String positions =
                        book.getValue()
                            .stream()
                            .map(String::valueOf)
                            .reduce(
                                (a, b) -> a + "," + b
                            )
                            .orElse("");

                String line =
                        book.getKey()
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
            String term)
            throws IOException {

        Map<Integer, List<Integer>> result =
                new HashMap<>();

        Path file =
                pathFor(term);

        if (!Files.exists(file)) {
            return result;
        }

        for (String line :
                Files.readAllLines(file)) {

            line = line.trim();

            if (line.isEmpty()) {
                continue;
            }

            String[] parts =
                    line.split("\\s+", 2);

            int bookId =
                    Integer.parseInt(parts[0]);

            List<Integer> positions =
                    new ArrayList<>();

            if (parts.length > 1
                    && !parts[1].isEmpty()) {

                for (String position :
                        parts[1].split(",")) {

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
}