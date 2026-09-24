package datalake;

import java.io.IOException;
import java.nio.file.Files;
import java.nio.file.Path;
import java.nio.file.Paths;
import java.util.List;
import java.util.Map;

public class IndexStorage {

    public static void save(
            InvertedIndex index,
            String date,
            String hour
    ) throws IOException {

        Path directory = Paths.get(
                "datalake",
                date,
                hour
        );

        Files.createDirectories(directory);

        Path indexFile = directory.resolve(
                "inverted_index.txt"
        );

        StringBuilder content = new StringBuilder();

        for (Map.Entry<String, Map<Integer, List<Integer>>> entry
                : index.getIndex().entrySet()) {

            String word = entry.getKey();

            content.append(word);

            for (Map.Entry<Integer, List<Integer>> document
                    : entry.getValue().entrySet()) {

                int documentId = document.getKey();

                List<Integer> positions =
                        document.getValue();

                content.append(" | ")
                       .append(documentId)
                       .append(" : ")
                       .append(positions);
            }

            content.append(System.lineSeparator());
        }

        Files.writeString(
                indexFile,
                content.toString()
        );

        System.out.println(
                "Índice guardado en: " + indexFile
        );
    }
}