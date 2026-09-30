package index;

import java.io.IOException;
import java.util.List;
import java.util.Map;

public interface IndexStore {

    void addBook(
            int bookId,
            Map<String, List<Integer>> tokens
    ) throws Exception;

    void flush() throws Exception;

    Map<Integer, List<Integer>> lookup(
            String term
    ) throws Exception;
}