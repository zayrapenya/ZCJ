package datalake;

import java.util.List;

public class BookManager {

    public static void processBook(
            int bookId,
            InvertedIndex index
    ) throws Exception {

        System.out.println();
        System.out.println("Procesando libro: " + bookId);


        String book =
                BookDownloader.downloadBook(bookId);

        System.out.println("Libro descargado.");


        DataLakeManager.saveBook(
                bookId,
                book
        );


        String body =
                BookProcessor.extractBody(book);

  
        List<String> words =
                TextProcessor.tokenize(body);

        System.out.println(
                "Número de palabras: "
                + words.size()
        );


        index.addDocument(
                bookId,
                words
        );

        System.out.println(
                "Libro añadido al índice."
        );
    }
}