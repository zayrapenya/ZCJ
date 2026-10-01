package benchmark;

import metadata.MetadataStore;

import java.nio.file.Files;
import java.nio.file.Path;
import java.sql.PreparedStatement;
import java.sql.ResultSet;
import java.util.ArrayList;
import java.util.List;

public class MetadataBenchmark {

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
                BenchmarkCommon.METADATA_SIZES) {

            run(
                    rows,
                    n
            );
        }

        BenchmarkCommon.writeCsv(
                Path.of("java_metadata.csv"),
                rows
        );

        System.out.println(
                "Generado: java_metadata.csv"
        );
    }

    private static void run(
            List<String[]> rows,
            int n
    ) throws Exception {

        Path root =
                BenchmarkCommon.benchmarkRoot(
                        "stage1_metadata_"
                                + n
                );

        Path database =
                root.resolve(
                        "metadata.db"
                );

        MetadataStore metadata =
                new MetadataStore(
                        database.toString()
                );

        long start =
                System.nanoTime();

        for (int i = 0; i < n; i++) {

            int sourceId =
                    BenchmarkCommon.sampleBook(i);

            int id =
                    BenchmarkCommon.syntheticBookId(i);

            metadata.save(
                    id,
                    BenchmarkCommon.sampleHeader(
                            sourceId
                    ),
                    Path.of(
                            "sample_data",
                            sourceId
                                    + ".body.txt"
                    )
            );
        }

        long insertTime =
                System.nanoTime() - start;

        double insertRate =
                n
                        / BenchmarkCommon.seconds(
                                insertTime
                        );

        add(
                rows,
                n,
                "insert_rows_per_sec",
                insertRate
        );

        /*
         * AUTHOR
         */
        String author =
                "Austen";

        start =
                System.nanoTime();

        try (
                ResultSet rs =
                        metadata.findByAuthor(
                                author
                        )
        ) {

            while (rs.next()) {
                rs.getInt("book_id");
            }
        }

        long authorTime =
                System.nanoTime() - start;

        add(
                rows,
                n,
                "query_by_author_ms",
                BenchmarkCommon.milliseconds(
                        authorTime
                )
        );

        /*
         * TITLE
         */
        String title =
                "Pride and Prejudice";

        start =
                System.nanoTime();

        try (
                ResultSet rs =
                        metadata.findPathByTitle(
                                title
                        )
        ) {

            while (rs.next()) {
                rs.getString("path");
            }
        }

        long titleTime =
                System.nanoTime() - start;

        add(
                rows,
                n,
                "query_path_by_title_ms",
                BenchmarkCommon.milliseconds(
                        titleTime
                )
        );

        /*
         * ID
         */
        start =
                System.nanoTime();

        try (
                ResultSet rs =
                        metadata.findById(
                                BenchmarkCommon
                                        .syntheticBookId(0)
                        )
        ) {

            while (rs.next()) {
                rs.getInt("book_id");
            }
        }

        long idTime =
                System.nanoTime() - start;

        add(
                rows,
                n,
                "query_by_id_ms",
                BenchmarkCommon.milliseconds(
                        idTime
                )
        );

        metadata.close();

        BenchmarkCommon.deleteDirectory(
                root
        );
    }

    private static void add(
            List<String[]> rows,
            int n,
            String metric,
            double value
    ) {

        rows.add(
                new String[]{
                        "java",
                        "metadata",
                        "sqlite",
                        String.valueOf(n),
                        metric,
                        BenchmarkCommon.format(
                                value
                        )
                }
        );
    }
}
