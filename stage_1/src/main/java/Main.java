import datalake.BookDownloader;
import datalake.DataLakeManager;
import datalake.TextProcessor;

import java.util.List;

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

            String body = datalake.BookProcessor.extractBody(book);

            List<String> words = TextProcessor.tokenize(body);

            System.out.println("Número de palabras: " + words.size());

            System.out.println("Primeras 20 palabras:");

            for (int i = 0; i < Math.min(20, words.size()); i++) {

                System.out.println(words.get(i));

            }

        } catch (Exception e) {

            e.printStackTrace();

        }
    }
}