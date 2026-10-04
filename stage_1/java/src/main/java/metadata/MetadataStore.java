package metadata;

import java.nio.file.Files;
import java.nio.file.Path;
import java.sql.Connection;
import java.sql.DriverManager;
import java.sql.PreparedStatement;
import java.sql.ResultSet;
import java.sql.SQLException;
import java.sql.Statement;
import java.util.regex.Matcher;
import java.util.regex.Pattern;

public class MetadataStore implements AutoCloseable {

    private final String url;
    private final Connection connection;

    private static final Pattern TITLE =
            Pattern.compile(
                    "^Title:\\s*(.+)$",
                    Pattern.MULTILINE
            );

    private static final Pattern AUTHOR =
            Pattern.compile(
                    "^Author:\\s*(.+)$",
                    Pattern.MULTILINE
            );

    private static final Pattern RELEASE_DATE =
            Pattern.compile(
                    "^Release [Dd]ate:\\s*(.+?)(?:\\s*\\[.*\\])?$",
                    Pattern.MULTILINE
            );

    private static final Pattern LANGUAGE =
            Pattern.compile(
                    "^Language:\\s*(.+)$",
                    Pattern.MULTILINE
            );

    public MetadataStore(
            String databasePath
    ) throws Exception {

        Path database =
                Path.of(databasePath);

        if (database.getParent() != null) {

            Files.createDirectories(
                    database.getParent()
            );
        }

        this.url =
                "jdbc:sqlite:" + databasePath;

        this.connection =
                DriverManager.getConnection(url);

        initialize();
    }

    private void initialize()
            throws SQLException {

        try (Statement statement =
                     connection.createStatement()) {

            statement.executeUpdate("""
                CREATE TABLE IF NOT EXISTS books (
                    book_id INTEGER PRIMARY KEY,
                    title TEXT,
                    author TEXT,
                    release_date TEXT,
                    language TEXT,
                    path TEXT
                )
                """);

            statement.executeUpdate("""
                CREATE INDEX IF NOT EXISTS idx_books_author
                ON books(author)
                """);

            statement.executeUpdate("""
                CREATE INDEX IF NOT EXISTS idx_books_title
                ON books(title)
                """);

            statement.executeUpdate("""
                CREATE INDEX IF NOT EXISTS idx_books_language
                ON books(language)
                """);
        }
    }

    private String extract(
            Pattern pattern,
            String header
    ) {

        Matcher matcher =
                pattern.matcher(header);

        if (matcher.find()) {
            return matcher.group(1).trim();
        }

        return null;
    }

    public void save(
            int bookId,
            String header,
            Path path
    ) throws SQLException {

        String title =
                extract(TITLE, header);

        String author =
                extract(AUTHOR, header);

        String releaseDate =
                extract(RELEASE_DATE, header);

        String language =
                extract(LANGUAGE, header);

        String sql = """
            INSERT OR REPLACE INTO books
            (book_id, title, author, release_date, language, path)
            VALUES (?, ?, ?, ?, ?, ?)
            """;

        try (PreparedStatement statement =
                     connection.prepareStatement(sql)) {

            statement.setInt(1, bookId);
            statement.setString(2, title);
            statement.setString(3, author);
            statement.setString(4, releaseDate);
            statement.setString(5, language);
            statement.setString(
                    6,
                    path.toString()
            );

            statement.executeUpdate();
        }
    }

    public void save(
            int bookId,
            String header,
            String path
    ) throws SQLException {

        save(
                bookId,
                header,
                Path.of(path)
        );
    }

    public ResultSet findById(int id)
            throws SQLException {

        PreparedStatement statement =
                connection.prepareStatement(
                        """
                        SELECT *
                        FROM books
                        WHERE book_id = ?
                        """
                );

        statement.setInt(1, id);

        return statement.executeQuery();
    }

    public ResultSet findByAuthor(
            String author
    ) throws SQLException {

        PreparedStatement statement =
                connection.prepareStatement(
                        """
                        SELECT *
                        FROM books
                        WHERE author LIKE ?
                        """
                );

        statement.setString(
                1,
                "%" + author + "%"
        );

        return statement.executeQuery();
    }

    public ResultSet findByLanguage(
            String language
    ) throws SQLException {

        PreparedStatement statement =
                connection.prepareStatement(
                        """
                        SELECT *
                        FROM books
                        WHERE language = ?
                        """
                );

        statement.setString(
                1,
                language
        );

        return statement.executeQuery();
    }

    public ResultSet findPathByTitle(
            String title
    ) throws SQLException {

        PreparedStatement statement =
                connection.prepareStatement(
                        """
                        SELECT path
                        FROM books
                        WHERE title = ?
                        """
                );

        statement.setString(
                1,
                title
        );

        return statement.executeQuery();
    }

    public int count()
            throws SQLException {

        try (
                Statement statement =
                        connection.createStatement();

                ResultSet result =
                        statement.executeQuery(
                                "SELECT COUNT(*) FROM books"
                        )
        ) {

            return result.getInt(1);
        }
    }

    public Connection openConnection() {
        return connection;
    }

    @Override
    public void close()
            throws SQLException {

        if (!connection.isClosed()) {
            connection.close();
        }
    }
}
