import control.ControlManager;
import control.Pipeline;
import datalake.BookDatalake;
import datalake.Datalake;
import datalake.DateTimeDatalake;
import datalake.RangeDatalake;
import index.HierarchicalIndex;
import index.IndexStore;
import index.MonolithicIndex;
import index.SQLiteIndex;
import index.SearchEngine;
import metadata.MetadataStore;

import java.nio.file.Files;
import java.nio.file.Path;
import java.sql.ResultSet;
import java.util.ArrayList;
import java.util.Comparator;
import java.util.List;
import java.util.Map;
import java.util.stream.Stream;

/*
 * Entry point of the Java implementation (run from the stage_1 folder).
 *
 * Options (before the command):
 *   --datalake time|book|range                 (default: time)
 *   --index monolithic|hierarchical|sqlite      (default: monolithic)
 *
 * Commands:
 *   sample                     ingest the books of sample_data/ through the control layer
 *   download <id> [<id> ...]   download books from Project Gutenberg into the datalake
 *   index                      index every downloaded book that is not indexed yet
 *   run <steps> [<id> ...]     run the control layer N steps (random ids if none given)
 *   search <word> [<word> ...] books containing every word (AND)
 *   phrase "<text>"            books containing the exact phrase
 *   metadata <id>              metadata of a book
 */
public class Main {

    private static final Path DATALAKE = Path.of("datalake");
    private static final Path DATAMARTS = Path.of("datamarts");
    private static final Path CONTROL = Path.of("control");
    private static final Path SAMPLE_DATA = Path.of("sample_data");

    public static void main(String[] args) throws Exception {

        String datalakeType = "time";
        String indexType = "monolithic";
        List<String> rest = new ArrayList<>();

        for (int i = 0; i < args.length; i++) {
            if (args[i].equals("--datalake") && i + 1 < args.length) {
                datalakeType = args[++i];
            } else if (args[i].equals("--index") && i + 1 < args.length) {
                indexType = args[++i];
            } else {
                rest.add(args[i]);
            }
        }

        if (rest.isEmpty()) {
            printUsage();
            return;
        }

        String command = rest.get(0);
        List<String> params = rest.subList(1, rest.size());

        IndexStore index = createIndex(indexType);
        MetadataStore metadata = new MetadataStore(DATAMARTS.resolve("metadata.db").toString());

        try {
            switch (command) {

                case "sample" -> {
                    List<Integer> ids = sampleIds();
                    Pipeline pipeline = pipeline(datalakeType, index, metadata, SAMPLE_DATA);
                    pipeline.run(2 * ids.size(), ids);
                }

                case "download" -> {
                    Pipeline pipeline = pipeline(datalakeType, index, metadata, null);
                    for (String id : params) {
                        pipeline.download(Integer.parseInt(id));
                    }
                }

                case "index" -> {
                    Pipeline pipeline = pipeline(datalakeType, index, metadata, null);
                    for (int id : pipeline.pending().stream().sorted().toList()) {
                        pipeline.indexBook(id);
                    }
                }

                case "run" -> {
                    int steps = params.isEmpty() ? 10 : Integer.parseInt(params.get(0));
                    List<Integer> ids = params.stream().skip(1).map(Integer::parseInt).toList();
                    Pipeline pipeline = pipeline(datalakeType, index, metadata, null);
                    pipeline.run(steps, ids);
                }

                case "search" -> {
                    SearchEngine engine = new SearchEngine(index);
                    Map<Integer, List<Integer>> results = engine.search(params.get(0));
                    for (String word : params.subList(1, params.size())) {
                        Map<Integer, List<Integer>> other = engine.search(word);
                        results.keySet().retainAll(other.keySet());
                    }
                    printResults(results, metadata);
                }

                case "phrase" -> {
                    SearchEngine engine = new SearchEngine(index);
                    printResults(engine.searchPhrase(String.join(" ", params)), metadata);
                }

                case "metadata" -> {
                    try (ResultSet row = metadata.findById(Integer.parseInt(params.get(0)))) {
                        if (row.next()) {
                            System.out.println(
                                    row.getInt("book_id") + " | "
                                            + row.getString("title") + " | "
                                            + row.getString("author") + " | "
                                            + row.getString("language") + " | "
                                            + row.getString("path")
                            );
                        } else {
                            System.out.println("Book not found.");
                        }
                    }
                }

                default -> printUsage();
            }
        } finally {
            index.close();
            metadata.close();
        }
    }

    private static Pipeline pipeline(
            String datalakeType,
            IndexStore index,
            MetadataStore metadata,
            Path sampleData
    ) throws Exception {

        return new Pipeline(
                createDatalake(datalakeType),
                index,
                metadata,
                new ControlManager(CONTROL),
                sampleData
        );
    }

    private static Datalake createDatalake(String type) {

        return switch (type) {
            case "time" -> new DateTimeDatalake(DATALAKE);
            case "book" -> new BookDatalake(DATALAKE);
            case "range" -> new RangeDatalake(DATALAKE);
            default -> throw new IllegalArgumentException("Unknown datalake: " + type);
        };
    }

    private static IndexStore createIndex(String type) throws Exception {

        return switch (type) {
            case "monolithic" -> new MonolithicIndex(DATAMARTS);
            case "hierarchical" -> new HierarchicalIndex(DATAMARTS);
            case "sqlite" -> new SQLiteIndex(DATAMARTS);
            default -> throw new IllegalArgumentException("Unknown index: " + type);
        };
    }

    private static List<Integer> sampleIds() throws Exception {

        try (Stream<Path> files = Files.list(SAMPLE_DATA)) {
            return files
                    .map(p -> p.getFileName().toString())
                    .filter(name -> name.startsWith("pg") && name.endsWith(".txt"))
                    .map(name -> Integer.parseInt(name.substring(2, name.length() - 4)))
                    .sorted()
                    .toList();
        }
    }

    private static void printResults(
            Map<Integer, List<Integer>> results,
            MetadataStore metadata
    ) throws Exception {

        if (results.isEmpty()) {
            System.out.println("No results.");
            return;
        }

        List<Map.Entry<Integer, List<Integer>>> sorted = new ArrayList<>(results.entrySet());
        sorted.sort(Comparator.comparingInt(e -> -e.getValue().size()));

        for (Map.Entry<Integer, List<Integer>> entry : sorted) {

            String title = "";

            try (ResultSet row = metadata.findById(entry.getKey())) {
                if (row.next()) {
                    title = row.getString("title") + " - " + row.getString("author");
                }
            }

            System.out.printf("%6d  %5d hits  %s%n", entry.getKey(), entry.getValue().size(), title);
        }
    }

    private static void printUsage() {
        System.out.println("""
                Usage: [--datalake time|book|range] [--index monolithic|hierarchical|sqlite] <command>
                  sample                     ingest the books of sample_data/
                  download <id> [<id> ...]   download books from Project Gutenberg
                  index                      index every pending book
                  run <steps> [<id> ...]     run the control layer N steps
                  search <word> [<word> ...] AND search
                  phrase "<text>"            exact phrase search
                  metadata <id>              metadata of a book
                """);
    }
}
