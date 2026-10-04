package datalake;

import java.io.IOException;
import java.nio.file.Files;
import java.nio.file.Path;
import java.time.LocalDateTime;
import java.time.format.DateTimeFormatter;
import java.util.HashSet;
import java.util.Set;
import java.util.stream.Stream;

public class DateTimeDatalake extends Datalake {

    private static final DateTimeFormatter DATE =
            DateTimeFormatter.ofPattern("yyyyMMdd");

    private static final DateTimeFormatter HOUR =
            DateTimeFormatter.ofPattern("HH");

    public DateTimeDatalake(Path root) {
        super(root);
    }

    public void save(
            int id,
            String header,
            String body
    ) throws IOException {

        save(
                id,
                header,
                body,
                LocalDateTime.now()
        );
    }

    public void save(
            int id,
            String header,
            String body,
            LocalDateTime time
    ) throws IOException {

        Path directory = root
                .resolve(time.format(DATE))
                .resolve(time.format(HOUR));

        Files.createDirectories(directory);

        Files.writeString(
                directory.resolve(id + ".header.txt"),
                header
        );

        Files.writeString(
                directory.resolve(id + ".body.txt"),
                body
        );
    }

    @Override
    public BookPaths locate(int id) throws IOException {

        if (!Files.exists(root)) {
            return null;
        }

        try (Stream<Path> dates = Files.list(root)) {

            for (Path date : dates.toList()) {

                if (!Files.isDirectory(date)) {
                    continue;
                }

                try (Stream<Path> hours =
                             Files.list(date)) {

                    for (Path hour : hours.toList()) {

                        if (!Files.isDirectory(hour)) {
                            continue;
                        }

                        Path header =
                                hour.resolve(id + ".header.txt");

                        Path body =
                                hour.resolve(id + ".body.txt");

                        if (Files.exists(header)
                                && Files.exists(body)) {

                            return new BookPaths(
                                    header,
                                    body
                            );
                        }
                    }
                }
            }
        }

        return null;
    }

    @Override
    public Set<Integer> bookIds()
            throws IOException {

        Set<Integer> result = new HashSet<>();

        if (!Files.exists(root)) {
            return result;
        }

        try (Stream<Path> files =
                     Files.walk(root)) {

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
