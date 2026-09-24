package datalake;

public class BookProcessor {

    private static final String START_MARKER =
            "*** START OF THE PROJECT GUTENBERG EBOOK";

    private static final String END_MARKER =
            "*** END OF THE PROJECT GUTENBERG EBOOK";

    public static String extractHeader(String book) {

        int start = book.indexOf(START_MARKER);

        if (start == -1) {
            throw new IllegalArgumentException(
                    "No se ha encontrado el inicio del libro"
            );
        }

        return book.substring(0, start).trim();
    }

    public static String extractBody(String book) {

        int start = book.indexOf(START_MARKER);
        int end = book.indexOf(END_MARKER);

        if (start == -1) {
            throw new IllegalArgumentException(
                    "No se ha encontrado el inicio del libro"
            );
        }

        if (end == -1) {
            throw new IllegalArgumentException(
                    "No se ha encontrado el final del libro"
            );
        }

        int bodyStart = start + START_MARKER.length();

        return book.substring(bodyStart, end).trim();
    }
}