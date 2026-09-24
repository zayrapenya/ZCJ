package datalake;

import java.util.List;

public class BookManager {

    public static void processBook(
            int bookId,
            InvertedIndex index,
            String date,
            String hour
    ) throws Exception {

        System.out.println();
        System.out.println(
                "================================"
        );

        System.out.println(
                "Procesando libro: " + bookId
        );

        System.out.println(
                "================================"
        );


        

        String book =
                BookDownloader.downloadBook(
                        bookId
                );

        System.out.println(
                "Libro descargado."
        );


        

        DataLakeManager.saveBook(
                bookId,
                book,
                date,
                hour
        );


        

        String body =
                BookProcessor.extractBody(
                        book
                );


        

        List<String> words =
                TextProcessor.tokenize(
                        body
                );

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