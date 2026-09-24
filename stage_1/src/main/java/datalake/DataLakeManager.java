package datalake;

import java.io.IOException;
import java.nio.file.Files;
import java.nio.file.Path;
import java.nio.file.Paths;

public class DataLakeManager {

    public static void saveBook(
            int bookId,
            String book,
            String date,
            String hour
    ) throws IOException {

        String header = BookProcessor.extractHeader(book);
        String body = BookProcessor.extractBody(book);

        Path directory = Paths.get(
                "datalake",
                date,
                hour
        );

        Files.createDirectories(directory);

        Path headerFile = directory.resolve(
                bookId + ".header.txt"
        );

        Path bodyFile = directory.resolve(
                bookId + ".body.txt"
        );

        Files.writeString(headerFile, header);
        Files.writeString(bodyFile, body);

        System.out.println("Datalake creado correctamente.");
        System.out.println("Header: " + headerFile);
        System.out.println("Body: " + bodyFile);
    }
}