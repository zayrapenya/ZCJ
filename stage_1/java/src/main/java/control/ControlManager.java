package control;

import java.io.IOException;
import java.nio.file.Files;
import java.nio.file.Path;
import java.util.HashSet;
import java.util.Set;
import java.util.stream.Collectors;

public class ControlManager {

    private final Path downloadedFile;
    private final Path indexedFile;
    private final Path failedFile;

    public ControlManager(Path root)
            throws IOException {

        Files.createDirectories(root);

        downloadedFile =
                root.resolve(
                        "downloaded_books.txt"
                );

        indexedFile =
                root.resolve(
                        "indexed_books.txt"
                );

        failedFile =
                root.resolve(
                        "failed_books.txt"
                );

        createIfNeeded(downloadedFile);
        createIfNeeded(indexedFile);
        createIfNeeded(failedFile);
    }

    private void createIfNeeded(Path path)
            throws IOException {

        if (!Files.exists(path)) {
            Files.createFile(path);
        }
    }

    public Set<Integer> read(Path file)
            throws IOException {

        if (!Files.exists(file)) {
            return new HashSet<>();
        }

        return Files.readAllLines(file)
                .stream()
                .map(String::trim)
                .filter(s -> !s.isEmpty())
                .map(Integer::parseInt)
                .collect(Collectors.toSet());
    }

    public Set<Integer> downloaded()
            throws IOException {

        return read(downloadedFile);
    }

    public Set<Integer> indexed()
            throws IOException {

        return read(indexedFile);
    }

    public Set<Integer> failed()
            throws IOException {

        return read(failedFile);
    }

    public void addDownloaded(int id)
            throws IOException {

        append(downloadedFile, id);
    }

    public void addIndexed(int id)
            throws IOException {

        append(indexedFile, id);
    }

    public void addFailed(int id)
            throws IOException {

        append(failedFile, id);
    }

    private void append(
            Path file,
            int id
    ) throws IOException {

        Set<Integer> existing =
                read(file);

        if (!existing.contains(id)) {

            Files.writeString(
                    file,
                    id + System.lineSeparator(),
                    java.nio.file.StandardOpenOption.APPEND
            );
        }
    }

    public Set<Integer> pending()
            throws IOException {

        Set<Integer> result =
                downloaded();

        result.removeAll(
                indexed()
        );

        return result;
    }

    public Integer lowestPending()
            throws IOException {

        return pending()
                .stream()
                .min(Integer::compareTo)
                .orElse(null);
    }

    public Integer nextNewBook(
            int startId
    ) throws IOException {

        Set<Integer> downloaded =
                downloaded();

        Set<Integer> failed =
                failed();

        int id = startId;

        while (
                downloaded.contains(id)
                        || failed.contains(id)
        ) {
            id++;
        }

        return id;
    }
}
