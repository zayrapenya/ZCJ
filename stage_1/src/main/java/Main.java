import datalake.*;

import java.nio.file.Path;

public class Main {

    public static void main(String[] args) throws Exception {

        int id = 1342;

        String header = "HEADER DE PRUEBA";
        String body = "This is the body of the book.";

        Path root = Path.of("datalake");

        Datalake dateTime =
                new DateTimeDatalake(root);

        Datalake books =
                new BookDatalake(root);

        Datalake ranges =
                new RangeDatalake(root);

        dateTime.save(id, header, body);
        books.save(id, header, body);
        ranges.save(id, header, body);

        System.out.println(
                "DateTime: "
                + dateTime.locate(id).getBody()
        );

        System.out.println(
                "Book: "
                + books.locate(id).getBody()
        );

        System.out.println(
                "Range: "
                + ranges.locate(id).getBody()
        );

        System.out.println(
                "DateTime IDs: "
                + dateTime.bookIds()
        );

        System.out.println(
                "Book IDs: "
                + books.bookIds()
        );

        System.out.println(
                "Range IDs: "
                + ranges.bookIds()
        );
    }
}