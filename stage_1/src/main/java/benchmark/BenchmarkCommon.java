package benchmark;

import java.io.IOException;
import java.nio.file.Files;
import java.nio.file.Path;
import java.nio.file.StandardOpenOption;
import java.util.ArrayList;
import java.util.List;
import java.util.Locale;

public final class BenchmarkCommon {

    public static final int[] SAMPLE_BOOKS = {
            11, 84, 98, 1342, 1661
    };

    public static final int[] DATALAKE_SIZES = {
            50, 100, 200
    };

    public static final int[] INDEX_SIZES = {
            5, 10, 20
    };

    public static final int[] METADATA_SIZES = {
            100, 1000, 10000
    };

    private BenchmarkCommon() {
    }

    // ---------------------------------------------------------
    // DIRECTORIOS
    // ---------------------------------------------------------

    public static Path benchmarkRoot(String name) throws IOException {

        Path root = Path.of(
                System.getProperty("java.io.tmpdir"),
                "stage1_benchmark",
                name
        );

        deleteDirectory(root);
        Files.createDirectories(root);

        return root;
    }

    public static void deleteDirectory(Path directory)
            throws IOException {

        if (directory == null || !Files.exists(directory)) {
            return;
        }

        try (var stream = Files.walk(directory)) {

            stream.sorted((a, b) -> b.compareTo(a))
                    .forEach(path -> {
                        try {
                            Files.deleteIfExists(path);
                        } catch (IOException e) {
                            throw new RuntimeException(
                                    "No se pudo eliminar: " + path,
                                    e
                            );
                        }
                    });
        }
    }

    // ---------------------------------------------------------
    // SAMPLE DATA
    // ---------------------------------------------------------

    public static int sampleBook(int index) {

        return SAMPLE_BOOKS[
                index % SAMPLE_BOOKS.length
        ];
    }

    public static int syntheticBookId(int index) {

        return 100000 + index;
    }

    public static Path sampleDataRoot() {

        Path root = Path.of("sample_data");

        if (!Files.exists(root)) {
            throw new IllegalStateException(
                    "No existe el directorio sample_data: "
                            + root.toAbsolutePath()
            );
        }

        return root;
    }

 public static String sampleHeader(int index)
        throws IOException {

    int bookId = sampleBook(index);

    Path file = sampleDataRoot()
            .resolve("pg" + bookId + ".txt");

    if (!Files.exists(file)) {
        throw new IOException(
                "No existe: " + file.toAbsolutePath()
        );
    }

    String book = Files.readString(file);

    return datalake.BookProcessor.extractHeader(book);
}

public static String sampleBody(int index)
        throws IOException {

    int bookId = sampleBook(index);

    Path file = sampleDataRoot()
            .resolve("pg" + bookId + ".txt");

    if (!Files.exists(file)) {
        throw new IOException(
                "No existe: " + file.toAbsolutePath()
        );
    }

    String book = Files.readString(file);

    return datalake.BookProcessor.extractBody(book);
}
    // ---------------------------------------------------------
    // TIEMPOS
    // ---------------------------------------------------------

    public static double seconds(long nanos) {

        return nanos / 1_000_000_000.0;
    }

    public static double milliseconds(long nanos) {

        return nanos / 1_000_000.0;
    }

    // ---------------------------------------------------------
    // FORMATO
    // ---------------------------------------------------------

    public static String format(double value) {

        return String.format(
                Locale.ROOT,
                "%.6f",
                value
        );
    }

    // ---------------------------------------------------------
    // CSV
    // ---------------------------------------------------------

    public static void writeCsv(
        Path file,
        List<String[]> rows
) throws IOException {

    Path parent = file.getParent();

    if (parent != null) {
        Files.createDirectories(parent);
    }

    StringBuilder csv = new StringBuilder();

    for (String[] row : rows) {

        for (int i = 0; i < row.length; i++) {

            if (i > 0) {
                csv.append(",");
            }

            String value = row[i] == null
                    ? ""
                    : row[i];

            if (value.contains(",")
                    || value.contains("\"")
                    || value.contains("\n")
                    || value.contains("\r")) {

                value = "\""
                        + value.replace("\"", "\"\"")
                        + "\"";
            }

            csv.append(value);
        }

        csv.append("\n");
    }

    Files.writeString(
            file,
            csv.toString(),
            StandardOpenOption.CREATE,
            StandardOpenOption.TRUNCATE_EXISTING
    );
}



    // ---------------------------------------------------------
    // ARCHIVOS / TAMAÑO
    // ---------------------------------------------------------

    public static long countFiles(Path root)
            throws IOException {

        if (!Files.exists(root)) {
            return 0;
        }

        try (var stream = Files.walk(root)) {

            return stream
                    .filter(Files::isRegularFile)
                    .count();
        }
    }

    public static long countDirectories(Path root)
            throws IOException {

        if (!Files.exists(root)) {
            return 0;
        }

        try (var stream = Files.walk(root)) {

            return stream
                    .filter(Files::isDirectory)
                    .count();
        }
    }

    public static long directoryBytes(Path root)
            throws IOException {

        if (!Files.exists(root)) {
            return 0;
        }

        try (var stream = Files.walk(root)) {

            return stream
                    .filter(Files::isRegularFile)
                    .mapToLong(path -> {

                        try {
                            return Files.size(path);
                        } catch (IOException e) {
                            return 0;
                        }

                    })
                    .sum();
        }
    }

    public static double megabytes(long bytes) {

        return bytes / 1024.0 / 1024.0;
    }

    // ---------------------------------------------------------
    // MEMORIA
    // ---------------------------------------------------------

    public static double usedMemoryMb() {

        Runtime runtime = Runtime.getRuntime();

        long used =
                runtime.totalMemory()
                        - runtime.freeMemory();

        return used / 1024.0 / 1024.0;
    }

    public static void forceGc() {

        System.gc();

        try {
            Thread.sleep(100);
        } catch (InterruptedException e) {
            Thread.currentThread().interrupt();
        }
    }

    // ---------------------------------------------------------
    // HELPERS
    // ---------------------------------------------------------

    public static List<Integer> syntheticIds(int nBooks) {

        List<Integer> ids = new ArrayList<>();

        for (int i = 0; i < nBooks; i++) {
            ids.add(syntheticBookId(i));
        }

        return ids;
    }
}
