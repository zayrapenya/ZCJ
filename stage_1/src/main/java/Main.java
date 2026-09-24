import datalake.BookDownloader;

public class Main {

    public static void main(String[] args) {

        try {

            String book = BookDownloader.downloadBook(1342);

            System.out.println(book.substring(0, 1000));

        } catch (Exception e) {

            e.printStackTrace();

        }
    }
}