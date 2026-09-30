package datalake;

import java.io.IOException;
import java.nio.file.Files;
import java.nio.file.Path;
import java.nio.file.Paths;
import java.time.LocalDateTime;
import java.time.format.DateTimeFormatter;

public class DataLakeManager {

    private static final DateTimeFormatter DATE_FORMAT =
            DateTimeFormatter.ofPattern("yyyyMMdd");

    public static Path saveBookTimeBased(
            int bookId,
            String header,
            String body
    ) throws IOException {

        LocalDateTime now = LocalDateTime.now();

        String date = now.format(DATE_FORMAT);
        int hour = now.getHour();

        Path directory = Paths.get(
                "datalake",
                date,
                String.format("%02d", hour)
        );

        Files.createDirectories(directory);

        Path headerPath =
                directory.resolve(bookId + ".header.txt");

        Path bodyPath =
                directory.resolve(bookId + ".body.txt");

        Files.writeString(headerPath, header);
        Files.writeString(bodyPath, body);

        return bodyPath;
    }

    public static Path saveBookBookBased(
            int bookId,
            String header,
            String body
    ) throws IOException {

        Path directory = Paths.get(
                "datalake",
                "books",
                String.valueOf(bookId)
        );

        Files.createDirectories(directory);

        Path headerPath =
                directory.resolve("header.txt");

        Path bodyPath =
                directory.resolve("body.txt");

        Files.writeString(headerPath, header);
        Files.writeString(bodyPath, body);

        return bodyPath;
    }

    public static Path saveBookRangeBased(
            int bookId,
            String header,
            String body
    ) throws IOException {

        int rangeStart = (bookId / 1000) * 1000;
        int rangeEnd = rangeStart + 999;

        String range = String.format(
                "%06d-%06d",
                rangeStart,
                rangeEnd
        );

        Path directory = Paths.get(
                "datalake",
                "ranges",
                range
        );

        Files.createDirectories(directory);

        Path headerPath =
                directory.resolve(bookId + ".header.txt");

        Path bodyPath =
                directory.resolve(bookId + ".body.txt");

        Files.writeString(headerPath, header);
        Files.writeString(bodyPath, body);

        return bodyPath;
    }
}