package index;

import java.io.IOException;
import java.nio.file.Files;
import java.nio.file.Path;
import java.sql.Connection;
import java.sql.DriverManager;
import java.sql.PreparedStatement;
import java.sql.ResultSet;
import java.sql.SQLException;
import java.sql.Statement;
import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;
import java.util.Map;

public class SQLiteIndex implements IndexStore {

    private final String url;
    private final Connection connection;

    public SQLiteIndex(Path directory)
            throws Exception {

        Files.createDirectories(directory);

        Path database =
                directory.resolve(
                        "inverted_index.db"
                );

        this.url =
                "jdbc:sqlite:" + database;

        this.connection =
                DriverManager.getConnection(url);

        initialize();
    }

    private void initialize()
            throws SQLException {

        try (Statement statement =
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
            Map<String, List<Integer>> tokens
    ) throws SQLException {

        String sql = """
            INSERT OR REPLACE INTO postings
            (term, book_id, positions)
            VALUES (?, ?, ?)
            """;

        boolean oldAutoCommit =
                connection.getAutoCommit();

        try {

            connection.setAutoCommit(false);

            try (PreparedStatement statement =
                         connection.prepareStatement(sql)) {

                for (
                        Map.Entry<String, List<Integer>> entry
                                : tokens.entrySet()
                ) {

                    StringBuilder positions =
                            new StringBuilder();

                    for (
                            int i = 0;
                            i < entry.getValue().size();
                            i++
                    ) {

                        if (i > 0) {
                            positions.append(",");
                        }

                        positions.append(
                                entry.getValue().get(i)
                        );
                    }

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
                            positions.toString()
                    );

                    statement.addBatch();
                }

                statement.executeBatch();
            }

            connection.commit();

        } catch (Exception e) {

            connection.rollback();

            throw e;

        } finally {

            connection.setAutoCommit(
                    oldAutoCommit
            );
        }
    }

    @Override
    public void flush() {
        /*
         * SQLite writes are committed in addBook().
         */
    }

    @Override
    public Map<Integer, List<Integer>> lookup(
            String term
    ) throws SQLException {

        Map<Integer, List<Integer>> result =
                new HashMap<>();

        String sql = """
            SELECT book_id, positions
            FROM postings
            WHERE term = ?
            """;

        try (PreparedStatement statement =
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

                    String raw =
                            rs.getString("positions");

                    List<Integer> positions =
                            new ArrayList<>();

                    if (raw != null
                            && !raw.isBlank()) {

                        for (
                                String position :
                                raw.split(",")
                        ) {

                            positions.add(
                                    Integer.parseInt(
                                            position
                                    )
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
    public void close()
            throws SQLException {

        if (!connection.isClosed()) {
            connection.close();
        }
    }

    @Override
    public long diskFiles()
            throws IOException {

        Path database =
                Path.of(
                        url.substring(
                                "jdbc:sqlite:".length()
                        )
                );

        return Files.exists(database)
                ? 1
                : 0;
    }

    @Override
    public long diskBytes()
            throws IOException {

        Path database =
                Path.of(
                        url.substring(
                                "jdbc:sqlite:".length()
                        )
                );

        if (!Files.exists(database)) {
            return 0;
        }

        return Files.size(database);
    }
}
