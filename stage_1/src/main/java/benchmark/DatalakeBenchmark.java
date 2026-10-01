package benchmark;

import control.ControlManager;
import datalake.BookPaths;
import datalake.BookDatalake;
import datalake.BookProcessor;
import datalake.DateTimeDatalake;
import datalake.Datalake;
import datalake.RangeDatalake;

import java.io.IOException;
import java.nio.file.Files;
import java.nio.file.Path;
import java.time.LocalDateTime;
import java.util.ArrayList;
import java.util.List;
import java.util.Random;
import java.util.Set;

public class DatalakeBenchmark {

    private static final String CSV =
            "java_datalake.csv";

    private static final int BOOKS_PER_HOUR =
            10;

    private static final int LOOKUPS =
            500;

    private static final LocalDateTime BASE_TIME =
            LocalDateTime.of(2025, 9, 25, 0, 0);

    public static void main(
            String[] args
    ) throws Exception {

        List<String[]> rows =
                new ArrayList<>();

        rows.add(
                new String[]{
                        "language",
                        "benchmark",
                        "structure",
                        "n_books",
                        "metric",
                        "value"
                }
        );

        for (int n :
                BenchmarkCommon.DATALAKE_SIZES) {

            run(
                    rows,
                    "time",
                    n
            );

            run(
                    rows,
                    "book",
                    n
            );

            run(
                    rows,
                    "range",
                    n
            );
        }

        BenchmarkCommon.writeCsv(
                Path.of(CSV),
                rows
        );

        System.out.println(
                "Generado: " + CSV
        );
    }

    private static void run(
            List<String[]> rows,
            String structure,
            int n
    ) throws Exception {

        Path root =
                BenchmarkCommon.benchmarkRoot(
                        "stage1_bench_java_"
                                + structure
                                + "_"
                                + n
                );

        Datalake datalake =
                createDatalake(
                        structure,
                        root
                );

        /*
         * Raw books are loaded once, outside the timed section.
         */
        List<String> rawBooks =
                loadRawBooks();

        /*
         * WRITE (split header/body + store)
         */
        long start =
                System.nanoTime();

        for (int i = 0; i < n; i++) {

            int id =
                    BenchmarkCommon.syntheticBookId(i);

            String raw =
                    rawBooks.get(
                            i % rawBooks.size()
                    );

            saveAt(
                    datalake,
                    id,
                    BookProcessor.extractHeader(raw),
                    BookProcessor.extractBody(raw),
                    ingestionTime(BASE_TIME, i)
            );
        }

        long writeTime =
                System.nanoTime() - start;

        double writeSeconds =
                BenchmarkCommon.seconds(
                        writeTime
                );

        double writeBooksPerSecond =
                n / writeSeconds;

        add(
                rows,
                structure,
                n,
                "write_books_per_sec",
                writeBooksPerSecond
        );

        /*
         * LOOKUP
         */
        int lookupCount =
                LOOKUPS;

        Random random =
                new Random(42);

        int[] lookupIds =
                new int[lookupCount];

        for (int i = 0; i < lookupCount; i++) {
            lookupIds[i] =
                    BenchmarkCommon.syntheticBookId(
                            random.nextInt(n)
                    );
        }

        start =
                System.nanoTime();

        for (int i = 0; i < lookupCount; i++) {

            int id =
                    lookupIds[i];

            BookPaths paths =
                    datalake.locate(id);

            if (paths == null) {
                throw new RuntimeException(
                        "No se pudo localizar "
                                + id
                );
            }
        }

        long lookupTime =
                System.nanoTime() - start;

        double lookupAverage =
                BenchmarkCommon.milliseconds(
                        lookupTime
                )
                        / lookupCount;

        add(
                rows,
                structure,
                n,
                "lookup_avg_ms",
                lookupAverage
        );

        /*
         * CONTROL INCREMENTAL
         */
        Path controlRoot =
                root.resolve("control");

        ControlManager control =
                new ControlManager(
                        controlRoot
                );

        Path downloaded =
                controlRoot.resolve(
                        "downloaded_books.txt"
                );

        Path indexed =
                controlRoot.resolve(
                        "indexed_books.txt"
                );

        StringBuilder downloadedText =
                new StringBuilder();

        StringBuilder indexedText =
                new StringBuilder();

        for (int i = 0; i < n; i++) {

            int id =
                    BenchmarkCommon.syntheticBookId(
                            i
                    );

            downloadedText
                    .append(id)
                    .append("\n");

            if (i < n / 2) {

                indexedText
                        .append(id)
                        .append("\n");
            }
        }

        Files.writeString(
                downloaded,
                downloadedText.toString()
        );

        Files.writeString(
                indexed,
                indexedText.toString()
        );

        start =
                System.nanoTime();

        Set<Integer> pending =
                control.pending();

        long controlTime =
                System.nanoTime() - start;

        add(
                rows,
                structure,
                n,
                "incremental_control_ms",
                BenchmarkCommon.milliseconds(
                        controlTime
                )
        );

        /*
         * INCREMENTAL SCAN
         */
        start =
                System.nanoTime();

        Set<Integer> existing =
                datalake.bookIds();

        long scanTime =
                System.nanoTime() - start;

        add(
                rows,
                structure,
                n,
                "incremental_scan_ms",
                BenchmarkCommon.milliseconds(
                        scanTime
                )
        );

        /*
         * STORAGE
         */
        long files =
                BenchmarkCommon.countFiles(
                        root
                );

        long dirs =
                BenchmarkCommon.countDirectories(
                        root
                );

        long bytes =
                BenchmarkCommon.directoryBytes(
                        root
                );

        add(
                rows,
                structure,
                n,
                "files",
                files
        );

        add(
                rows,
                structure,
                n,
                "dirs",
                dirs
        );

        add(
                rows,
                structure,
                n,
                "megabytes",
                BenchmarkCommon.megabytes(
                        bytes
                )
        );

        /*
         * RECOVERY
         *
         * Simulate an interrupted execution:
         * all downloaded, only half indexed.
         */
        start =
                System.nanoTime();

        Set<Integer> recoveryPending =
                control.pending();

        long recoveryTime =
                System.nanoTime() - start;

        int duplicated =
                0;

        int lost =
                0;

        /*
         * Existing datalake IDs must all be
         * represented in downloaded control.
         */
        Set<Integer> downloadedIds =
                control.downloaded();

        for (Integer id : existing) {

            if (!downloadedIds.contains(id)) {
                lost++;
            }
        }

        /*
         * Pending books are exactly the books
         * downloaded but not indexed.
         */
        for (Integer id :
                recoveryPending) {

            if (control.indexed().contains(id)) {
                duplicated++;
            }
        }

        add(
                rows,
                structure,
                n,
                "recovery_ms",
                BenchmarkCommon.milliseconds(
                        recoveryTime
                )
        );

        add(
                rows,
                structure,
                n,
                "recovery_duplicated_books",
                duplicated
        );

        add(
                rows,
                structure,
                n,
                "recovery_lost_books",
                lost
        );

        BenchmarkCommon.deleteDirectory(
                root
        );
    }

    private static Datalake createDatalake(
            String structure,
            Path root
    ) {

        switch (structure) {

            case "time":
                return new DateTimeDatalake(root);

            case "book":
                return new BookDatalake(root);

            case "range":
                return new RangeDatalake(root);

            default:
                throw new IllegalArgumentException(
                        "Estructura desconocida"
                );
        }
    }

    private static List<String> loadRawBooks()
            throws IOException {

        List<String> rawBooks =
                new ArrayList<>();

        for (int bookId : BenchmarkCommon.SAMPLE_BOOKS) {
            rawBooks.add(
                    Files.readString(
                            BenchmarkCommon.sampleDataRoot()
                                    .resolve("pg" + bookId + ".txt")
                    )
            );
        }

        return rawBooks;
    }

    /*
     * Simulated ingestion rate for the time-based layout,
     * the same as in the Python benchmark.
     */
    private static LocalDateTime ingestionTime(
            LocalDateTime base,
            int index
    ) {

        return base.plusHours(
                index / BOOKS_PER_HOUR
        );
    }

    private static void saveAt(
            Datalake datalake,
            int id,
            String header,
            String body,
            LocalDateTime time
    ) throws IOException {

        if (datalake instanceof DateTimeDatalake) {

            ((DateTimeDatalake) datalake).save(
                    id,
                    header,
                    body,
                    time
            );

        } else {

            datalake.save(
                    id,
                    header,
                    body
            );
        }
    }

    private static void add(
            List<String[]> rows,
            String structure,
            int n,
            String metric,
            double value
    ) {

        rows.add(
                new String[]{
                        "java",
                        "datalake",
                        structure,
                        String.valueOf(n),
                        metric,
                        BenchmarkCommon.format(
                                value
                        )
                }
        );
    }
}
