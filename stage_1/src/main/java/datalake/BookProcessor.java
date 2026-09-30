package datalake;

public class BookProcessor {

    private static final String START_MARKER =
            "*** START OF THE PROJECT GUTENBERG EBOOK";

    private static final String END_MARKER =
            "*** END OF THE PROJECT GUTENBERG EBOOK";

    public static String extractHeader(String book) {

   
        book = book.replace("\r\n", "\n");

        int start = book.indexOf(START_MARKER);
        int end = book.indexOf(END_MARKER);

        if (start == -1 || end == -1) {
            throw new RuntimeException(
                    "Marcadores de Gutenberg no encontrados"
            );
        }

        return book.substring(0, start).trim();
    }

    public static String extractBody(String book) {


        book = book.replace("\r\n", "\n");

        int start = book.indexOf(START_MARKER);
        int end = book.indexOf(END_MARKER);

        if (start == -1 || end == -1) {
            throw new RuntimeException(
                    "Marcadores de Gutenberg no encontrados"
            );
        }


        int bodyStart = book.indexOf("\n", start);

        if (bodyStart == -1 || bodyStart >= end) {
            throw new RuntimeException(
                    "No se pudo localizar correctamente el inicio del body"
            );
        }

        return book.substring(bodyStart + 1, end).trim();
    }
}