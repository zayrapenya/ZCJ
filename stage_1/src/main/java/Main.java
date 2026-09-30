import datalake.BookDownloader;
import datalake.BookProcessor;

public class Main {

    public static void main(String[] args) {

        try {

            int bookId = 1342;

            System.out.println("Descargando libro " + bookId + "...");

            String book = BookDownloader.downloadBook(bookId);

            System.out.println("Libro descargado.");
            System.out.println();

            String header = BookProcessor.extractHeader(book);
            String body = BookProcessor.extractBody(book);

            System.out.println("HEADER:");
            System.out.println("-------------------------");
            System.out.println(header.substring(
                    0,
                    Math.min(1000, header.length())
            ));

            System.out.println();
            System.out.println("BODY:");
            System.out.println("-------------------------");
            System.out.println(body.substring(
                    0,
                    Math.min(1000, body.length())
            ));

            System.out.println();
            System.out.println("Longitud del body: " + body.length());

        } catch (Exception e) {

            e.printStackTrace();

        }
    }
}