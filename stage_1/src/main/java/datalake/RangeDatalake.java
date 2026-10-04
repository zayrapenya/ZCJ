package datalake;

import java.io.IOException;
import java.nio.file.Files;
import java.nio.file.Path;
import java.util.HashSet;
import java.util.Set;
import java.util.stream.Stream;

public class RangeDatalake extends Datalake {

    public RangeDatalake(Path root) {
        super(root);
    }

    private String rangeFor(int id) {

        int start = (id / 1000) * 1000;
        int end = start + 999;

        return String.format(
                "%06d-%06d",
                start,
                end
        );
    }

    @Override
    public void save(
            int id,
            String header,
            String body
    ) throws IOException {

        Path directory =
                root
                        .resolve("ranges")
                        .resolve(rangeFor(id));

        Files.createDirectories(directory);

        Files.writeString(
                directory.resolve(
                        id + ".header.txt"
                ),
                header
        );

        Files.writeString(
                directory.resolve(
                        id + ".body.txt"
                ),
                body
        );
    }

    @Override
    public BookPaths locate(int id)
            throws IOException {

        Path directory =
                root
                        .resolve("ranges")
                        .resolve(rangeFor(id));

        Path header =
                directory.resolve(
                        id + ".header.txt"
                );

        Path body =
                directory.resolve(
                        id + ".body.txt"
                );

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

        Path ranges =
                root.resolve("ranges");

        if (!Files.exists(ranges)) {
            return result;
        }

        try (Stream<Path> files =
                     Files.walk(ranges)) {

            files
                    .filter(Files::isRegularFile)
                    .filter(p ->
                            p.getFileName()
                                    .toString()
                                    .endsWith(".body.txt"))
                    .forEach(p -> {

                        String name =
                                p.getFileName()
                                        .toString()
                                        .replace(
                                                ".body.txt",
                                                ""
                                        );

                        try {
                            result.add(
                                    Integer.parseInt(name)
                            );
                        } catch (NumberFormatException ignored) {
                        }
                    });
        }

        return result;
    }
}
