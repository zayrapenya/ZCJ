package benchmark;

import index.HierarchicalIndex;
import index.IndexStore;
import index.MonolithicIndex;
import index.SQLiteIndex;
import index.SearchEngine;
import index.Tokenizer;

import java.nio.file.Files;
import java.nio.file.Path;
import java.util.ArrayList;
import java.util.List;
import java.util.Map;

public class IndexBenchmark {

    private static final String[] WORD_QUERIES = {
            "darcy",
            "monster",
            "alice",
            "holmes",
            "love",
            "the",
            "revolution",
            "creature",
            "queen",
            "zzzz"
    };

    private static final String[][] AND_QUERIES = {
            {"darcy", "love"},
            {"monster", "night"},
            {"alice", "queen"},
            {"holmes", "watson"}
    };

    private static final String[] PHRASE_QUERIES = {
            "mr darcy",
            "sherlock holmes",
            "the white rabbit",
            "it was the best of times"
    };

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
                BenchmarkCommon.INDEX_SIZES) {

            run(
                    rows,
                    "monolithic",
                    n
            );

            run(
                    rows,
                    "hierarchical",
                    n
            );

            run(
                    rows,
                    "sqlite",
                    n
            );
        }

        BenchmarkCommon.writeCsv(
                Path.of("java_index.csv"),
                rows
        );

        System.out.println(
                "Generado: java_index.csv"
        );
    }

    private static void run(
            List<String[]> rows,
            String structure,
            int n
    ) throws Exception {

        Path root =
                BenchmarkCommon.benchmarkRoot(
                        "stage1_index_"
                                + structure
                                + "_"
                                + n
                );

        IndexStore index =
                createIndex(
                        structure,
                        root
                );

        long start =
                System.nanoTime();

        long memoryBefore =
                usedMemory();

        for (int i = 0; i < n; i++) {

            int sourceId =
                    BenchmarkCommon.sampleBook(i);

            int bookId =
                    BenchmarkCommon.syntheticBookId(i);

            String body =
                    BenchmarkCommon.sampleBody(
                            sourceId
                    );

            Map<String, List<Integer>> tokens =
                    Tokenizer.tokenize(body);

            index.addBook(
                    bookId,
                    tokens
            );
        }

        index.flush();

        long indexingTime =
                System.nanoTime() - start;

        long memoryAfter =
                usedMemory();

        double indexingSeconds =
                BenchmarkCommon.seconds(
                        indexingTime
                );

        double peakRamMb =
                BenchmarkCommon.megabytes(
                        Math.max(
                                0,
                                memoryAfter
                                        - memoryBefore
                        )
                );

        add(
                rows,
                structure,
                n,
                "indexing_sec",
                indexingSeconds
        );

        add(
                rows,
                structure,
                n,
                "indexing_peak_ram_mb",
                peakRamMb
        );

        /*
         * Queries on a fresh instance.
         */
        index.close();

        IndexStore coldIndex =
                createIndex(
                        structure,
                        root
                );

        SearchEngine search =
                new SearchEngine(
                        coldIndex
                );

        start =
                System.nanoTime();

        for (String query :
                WORD_QUERIES) {

            search.search(query);
        }

        for (String[] query :
                AND_QUERIES) {

            search.searchAnd(
                    query[0],
                    query[1]
            );
        }

        for (String phrase :
                PHRASE_QUERIES) {

            search.searchPhrase(
                    phrase
            );
        }

        long coldTime =
                System.nanoTime() - start;

        add(
                rows,
                structure,
                n,
                "query_cold_total_ms",
                BenchmarkCommon.milliseconds(
                        coldTime
                )
        );

        /*
         * Average query time.
         */
        int queryCount =
                WORD_QUERIES.length
                        + AND_QUERIES.length
                        + PHRASE_QUERIES.length;

        start =
                System.nanoTime();

        for (String query :
                WORD_QUERIES) {

            search.search(query);
        }

        for (String[] query :
                AND_QUERIES) {

            search.searchAnd(
                    query[0],
                    query[1]
            );
        }

        for (String phrase :
                PHRASE_QUERIES) {

            search.searchPhrase(
                    phrase
            );
        }

        long queryTime =
                System.nanoTime() - start;

        add(
                rows,
                structure,
                n,
                "query_avg_ms",
                BenchmarkCommon.milliseconds(
                        queryTime
                ) / queryCount
        );

        /*
         * Update one book.
         */
        int updateId =
                BenchmarkCommon.syntheticBookId(0);

        int sourceId =
                BenchmarkCommon.sampleBook(0);

        Map<String, List<Integer>> tokens =
                Tokenizer.tokenize(
                        BenchmarkCommon.sampleBody(
                                sourceId
                        )
                );

        start =
                System.nanoTime();

        coldIndex.addBook(
                updateId,
                tokens
        );

        coldIndex.flush();

        long updateTime =
                System.nanoTime() - start;

        add(
                rows,
                structure,
                n,
                "update_one_book_sec",
                BenchmarkCommon.seconds(
                        updateTime
                )
        );

        /*
         * Disk.
         */
        add(
                rows,
                structure,
                n,
                "disk_files",
                coldIndex.diskFiles()
        );

        add(
                rows,
                structure,
                n,
                "disk_mb",
                BenchmarkCommon.megabytes(
                        coldIndex.diskBytes()
                )
        );

        coldIndex.close();

        BenchmarkCommon.deleteDirectory(
                root
        );
    }

    private static IndexStore createIndex(
            String structure,
            Path root
    ) throws Exception {

        return switch (structure) {

            case "monolithic" ->
                    new MonolithicIndex(root);

            case "hierarchical" ->
                    new HierarchicalIndex(root);

            case "sqlite" ->
                    new SQLiteIndex(root);

            default ->
                    throw new IllegalArgumentException(
                            "Estructura desconocida: "
                                    + structure
                    );
        };
    }

    private static long usedMemory() {

        Runtime runtime =
                Runtime.getRuntime();

        return runtime.totalMemory()
                - runtime.freeMemory();
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
                        "index",
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
