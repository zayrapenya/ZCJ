package datalake;

import java.net.URI;
import java.net.http.HttpClient;
import java.net.http.HttpRequest;
import java.net.http.HttpResponse;

public class BookDownloader {

    private static final HttpClient CLIENT = HttpClient.newHttpClient();

    private BookDownloader() {
    }

    public static String downloadBook(int bookId) throws Exception {

        String url =
                "https://www.gutenberg.org/cache/epub/"
                        + bookId
                        + "/pg"
                        + bookId
                        + ".txt";

        HttpRequest request = HttpRequest.newBuilder()
                .uri(URI.create(url))
                .GET()
                .build();

        HttpResponse<String> response =
                CLIENT.send(
                        request,
                        HttpResponse.BodyHandlers.ofString()
                );

        if (response.statusCode() != 200) {
            throw new RuntimeException(
                    "Error descargando el libro "
                            + bookId
                            + ". HTTP "
                            + response.statusCode()
            );
        }

        return response.body();
    }
}
