package datalake;

import java.io.IOException;
import java.nio.file.Path;
import java.util.Set;

public abstract class Datalake {

    protected final Path root;

    protected Datalake(Path root) {
        this.root = root;
    }

    public abstract void save(
            int id,
            String header,
            String body
    ) throws IOException;

    public abstract BookPaths locate(
            int id
    ) throws IOException;

    public abstract Set<Integer> bookIds()
            throws IOException;

    public Path getRoot() {
        return root;
    }
}
