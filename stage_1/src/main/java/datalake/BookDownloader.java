package datalake;

import java.net.URI;
import java.net.http.HttpClient;
import java.net.http.HttpRequest;
import java.net.http.HttpResponse;

public class BookDownloader {

    public static String downloadBook(int bookId) throws Exception {

        String url = "https://www.gutenberg.org/cache/epub/"
                + bookId
                + "/pg"
                + bookId
                + ".txt";

        HttpClient client = HttpClient.newHttpClient();

        HttpRequest request = HttpRequest.newBuilder()
                .uri(URI.create(url))
                .GET()
                .build();

        HttpResponse<String> response = client.send(
                request,
                HttpResponse.BodyHandlers.ofString()
        );

        return response.body();
    }
}