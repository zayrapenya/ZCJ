package index;

import java.nio.file.*;
import java.sql.*;
import java.util.*;

public class SQLiteIndex
        implements IndexStore {

    private final String url;

    public SQLiteIndex(String databasePath)
            throws SQLException {

        this.url =
                "jdbc:sqlite:" + databasePath;

        initialize();
    }

    private void initialize()
            throws SQLException {

        Path path =
                Path.of(
                    url.substring("jdbc:sqlite:".length())
                );

        if (path.getParent() != null) {

            try {
                Files.createDirectories(
                        path.getParent()
                );
            } catch (Exception e) {
                throw new SQLException(e);
            }
        }

        try (Connection connection =
                     DriverManager.getConnection(url);
             Statement statement =
                     connection.createStatement()) {

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
            Map<String, List<Integer>> tokens)
            throws SQLException {

        String sql = """
            INSERT OR REPLACE INTO postings
            (term, book_id, positions)
            VALUES (?, ?, ?)
        """;

        try (Connection connection =
                     DriverManager.getConnection(url);
             PreparedStatement statement =
                     connection.prepareStatement(sql)) {

            connection.setAutoCommit(false);

            for (var entry :
                    tokens.entrySet()) {

                String positions =
                        entry.getValue()
                                .stream()
                                .map(String::valueOf)
                                .reduce(
                                    (a, b) -> a + "," + b
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

            statement.executeBatch();

            connection.commit();
        }
    }

    @Override
    public void flush() {
        // SQLite escribe inmediatamente.
    }

    @Override
    public Map<Integer, List<Integer>> lookup(
            String term)
            throws SQLException {

        Map<Integer, List<Integer>> result =
                new HashMap<>();

        String sql = """
            SELECT book_id, positions
            FROM postings
            WHERE term = ?
        """;

        try (Connection connection =
                     DriverManager.getConnection(url);
             PreparedStatement statement =
                     connection.prepareStatement(sql)) {

            statement.setString(
                    1,
                    term.toLowerCase()
            );

            try (ResultSet rs =
                         statement.executeQuery()) {

                while (rs.next()) {

                    int bookId =
                            rs.getInt("book_id");

                    String positionsText =
                            rs.getString("positions");

                    List<Integer> positions =
                            new ArrayList<>();

                    if (positionsText != null
                            && !positionsText.isEmpty()) {

                        for (String position :
                                positionsText.split(",")) {

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
}