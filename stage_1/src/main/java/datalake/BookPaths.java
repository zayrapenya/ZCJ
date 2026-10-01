package datalake;

import java.nio.file.Path;

public class BookPaths {

    private final Path header;
    private final Path body;

    public BookPaths(Path header, Path body) {
        this.header = header;
        this.body = body;
    }

    public Path getHeader() {
        return header;
    }

    public Path getBody() {
        return body;
    }
}
