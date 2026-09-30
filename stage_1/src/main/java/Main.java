import datalake.BookDatalake;
import datalake.BookPaths;
import datalake.DateTimeDatalake;
import datalake.Datalake;
import datalake.RangeDatalake;

import java.nio.file.Path;

public class Main {

    public static void main(String[] args) {

        try {

            int id = 1342;

            String header = "Title: Pride and Prejudice";
            String body = "Mr Darcy loves Elizabeth.";

            Datalake time =
                    new DateTimeDatalake(
                            Path.of("datalake")
                    );

            Datalake book =
                    new BookDatalake(
                            Path.of("datalake")
                    );

            Datalake range =
                    new RangeDatalake(
                            Path.of("datalake")
                    );

            time.save(id, header, body);
            book.save(id, header, body);
            range.save(id, header, body);

            System.out.println(
                    "TIME: "
                            + time.locate(id).getBody()
            );

            System.out.println(
                    "BOOK: "
                            + book.locate(id).getBody()
            );

            System.out.println(
                    "RANGE: "
                            + range.locate(id).getBody()
            );

            System.out.println();
            System.out.println("DATALAKE OK");

        } catch (Exception e) {
            e.printStackTrace();
        }
    }
}