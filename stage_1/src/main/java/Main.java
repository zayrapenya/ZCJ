import datalake.BookDownloader;
import datalake.DataLakeManager;

public class Main {

    public static void main(String[] args) {

        try {

            int bookId = 1342;

            System.out.println("Descargando libro...");

            String book = BookDownloader.downloadBook(bookId);

            System.out.println("Libro descargado.");
            System.out.println("Tamaño: " + book.length() + " caracteres");

            DataLakeManager.saveBook(
                    bookId,
                    book,
                    "20260924",
                    "10"
            );

        } catch (Exception e) {

            e.printStackTrace();

        }
    }
}