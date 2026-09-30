package index;

import java.io.IOException;
import java.nio.file.Files;
import java.nio.file.Path;
import java.sql.*;
import java.util.ArrayList;
import java.util.Collections;
import java.util.HashMap;
import java.util.List;
import java.util.Map;

public class SQLiteIndex implements IndexStore {

    private final Path database;

    private final String url;

    private final Map<Integer, Map<String, List<Integer>>> pending =
            new HashMap<>();

    public SQLiteIndex(Path root)
            throws Exception {

        Files.createDirectories(root);

        database =
                root.resolve(
                        "inverted_index.db"
                );

        url =
                "jdbc:sqlite:" + database;

        initialize();
    }

    private void initialize()
            throws SQLException {

        try (
                Connection connection =
                        DriverManager.getConnection(url);

                Statement statement =
                        connection.createStatement()
        ) {

            statement.executeUpdate("""
                CREATE TABLE IF NOT EXISTS postings (
                    term TEXT,
                    book_id INTEGER,
                    positions TEXT,
                    PRIMARY KEY (term, book_id)
                ) WITHOUT ROWID
                """);
        }
    }

    @Override
    public void addBook(
            int bookId,
            Map<String, List<Integer>> tokens
    ) {

        pending.put(
                bookId,
                new HashMap<>(tokens)
        );
    }

    @Override
    public void flush()
            throws SQLException {

        if (pending.isEmpty()) {
            return;
        }

        try (
                Connection connection =
                        DriverManager.getConnection(url)
        ) {

            connection.setAutoCommit(false);

            String sql = """
                INSERT OR REPLACE INTO postings
                (term, book_id, positions)
                VALUES (?, ?, ?)
                """;

            try (
                    PreparedStatement statement =
                            connection.prepareStatement(sql)
            ) {

                for (
                        Map.Entry<Integer, Map<String, List<Integer>>> book
                                : pending.entrySet()
                ) {

                    int bookId =
                            book.getKey();

                    for (
                            Map.Entry<String, List<Integer>> entry
                                    : book.getValue().entrySet()
                    ) {

                        String positions =
                                entry.getValue()
                                        .stream()
                                        .map(String::valueOf)
                                        .reduce(
                                                (a, b) ->
                                                        a + "," + b
                                        )
                                        .orElse("");

                        statement.setString(
                                1,
                                entry.getKey()
                        );

                        statement.setInt(
                                2,
                                bookId
                        );

                        statement.setString(
                                3,
                                positions
                        );

                        statement.addBatch();
                    }
                }

                statement.executeBatch();
            }

            connection.commit();
        }

        pending.clear();
    }

    @Override
    public Map<Integer, List<Integer>> lookup(
            String term
    ) throws SQLException {

        String sql = """
            SELECT book_id, positions
            FROM postings
            WHERE term = ?
            """;

        Map<Integer, List<Integer>> result =
                new HashMap<>();

        try (
                Connection connection =
                        DriverManager.getConnection(url);

                PreparedStatement statement =
                        connection.prepareStatement(sql)
        ) {

            statement.setString(
                    1,
                    term.toLowerCase()
            );

            try (
                    ResultSet rs =
                            statement.executeQuery()
            ) {

                while (rs.next()) {

                    int bookId =
                            rs.getInt("book_id");

                    String positionsText =
                            rs.getString("positions");

                    List<Integer> positions =
                            new ArrayList<>();

                    if (
                            positionsText != null
                                    && !positionsText.isEmpty()
                    ) {

                        for (
                                String position :
                                positionsText.split(",")
                        ) {

                            positions.add(
                                    Integer.parseInt(position)
                            );
                        }
                    }

                    result.put(
                            bookId,
                            positions
                    );
                }
            }
        }

        return result;
    }

    @Override
    public void close() {
    }

    @Override
    public long diskFiles() {
        return database.toFile().exists()
                ? 1
                : 0;
    }

    @Override
    public long diskBytes()
            throws IOException {

        return Files.exists(database)
                ? Files.size(database)
                : 0;
    }
}