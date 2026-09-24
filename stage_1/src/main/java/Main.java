import datalake.BookDownloader;
import datalake.BookProcessor;
import datalake.DataLakeManager;
import datalake.InvertedIndex;
import datalake.TextProcessor;

import java.util.List;
import java.util.Map;

public class Main {

    public static void main(String[] args) {

        try {

            int bookId = 1342;

            System.out.println("Descargando libro...");

            String book = BookDownloader.downloadBook(bookId);

            System.out.println("Libro descargado.");

            DataLakeManager.saveBook(
                    bookId,
                    book
            );

            String body = BookProcessor.extractBody(book);

            List<String> words = TextProcessor.tokenize(body);

            System.out.println(
                    "Número de palabras: " + words.size()
            );

            InvertedIndex index = new InvertedIndex();

            index.addDocument(
                    bookId,
                    words
            );

            System.out.println(
                    "Número de palabras diferentes: "
                    + index.size()
            );

            String[] wordsToSearch = {
            "darcy",
            "elizabeth",
            "marriage",
            "love",
            "truth"
    };

    for (String word : wordsToSearch) {

        Map<Integer, List<Integer>> documents =
            index.search(word);

        System.out.println(
            word + " → " + documents
        );
}

        } catch (Exception e) {

            e.printStackTrace();

        }
    }
}