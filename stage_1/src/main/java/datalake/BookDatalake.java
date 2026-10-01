package datalake;

import java.io.IOException;
import java.nio.file.Files;
import java.nio.file.Path;
import java.util.HashSet;
import java.util.Set;
import java.util.stream.Stream;

public class BookDatalake extends Datalake {

    public BookDatalake(Path root) {
        super(root);
    }

    @Override
    public void save(
            int id,
            String header,
            String body
    ) throws IOException {

        Path directory =
                root
                        .resolve("books")
                        .resolve(String.valueOf(id));

        Files.createDirectories(directory);

        Files.writeString(
                directory.resolve("header.txt"),
                header
        );

        Files.writeString(
                directory.resolve("body.txt"),
                body
        );
    }

    @Override
    public BookPaths locate(int id)
            throws IOException {

        Path directory =
                root
                        .resolve("books")
                        .resolve(String.valueOf(id));

        Path header =
                directory.resolve("header.txt");

        Path body =
                directory.resolve("body.txt");

        if (Files.exists(header)
                && Files.exists(body)) {

            return new BookPaths(
                    header,
                    body
            );
        }

        return null;
    }

    @Override
    public Set<Integer> bookIds()
            throws IOException {

        Set<Integer> result = new HashSet<>();

        Path books =
                root.resolve("books");

        if (!Files.exists(books)) {
            return result;
        }

        try (Stream<Path> dirs =
                     Files.list(books)) {

            for (Path dir : dirs.toList()) {

                if (!Files.isDirectory(dir)) {
                    continue;
                }

                try {
                    int id = Integer.parseInt(
                            dir.getFileName()
                                    .toString()
                    );

                    if (Files.exists(
                            dir.resolve("body.txt"))) {

                        result.add(id);
                    }

                } catch (NumberFormatException ignored) {
                }
            }
        }

        return result;
    }
}
