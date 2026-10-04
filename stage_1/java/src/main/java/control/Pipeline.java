package control;

import datalake.BookDownloader;
import datalake.BookPaths;
import datalake.BookProcessor;
import datalake.Datalake;
import index.IndexStore;
import index.Tokenizer;
import metadata.MetadataStore;

import java.nio.file.Files;
import java.nio.file.Path;
import java.util.List;
import java.util.Random;
import java.util.Set;

/*
 * Control layer: decides at each step whether to index a pending book
 * or to download a new one, using the control files to avoid duplicates.
 *
 * A book id is written to a control file only after the step has finished,
 * so an interrupted run can simply be restarted.
 */
public class Pipeline {

    private static final int TOTAL_BOOKS =
            70000;

    private final Datalake datalake;
    private final IndexStore index;
    private final MetadataStore metadata;
    private final ControlManager control;
    private final Path sampleData;

    /*
     * sampleData: if not null, books are read from <sampleData>/pg<ID>.txt
     * instead of being downloaded from Project Gutenberg.
     */
    public Pipeline(
            Datalake datalake,
            IndexStore index,
            MetadataStore metadata,
            ControlManager control,
            Path sampleData
    ) {
        this.datalake = datalake;
        this.index = index;
        this.metadata = metadata;
        this.control = control;
        this.sampleData = sampleData;
    }

    public boolean download(int bookId) throws Exception {

        if (control.downloaded().contains(bookId)) {
            System.out.println("[CONTROL] Book " + bookId + " already downloaded, skipping.");
            return true;
        }

        String header;
        String body;

        try {
            String text = fetch(bookId);
            header = BookProcessor.extractHeader(text);
            body = BookProcessor.extractBody(text);
        } catch (Exception e) {
            System.out.println("[CONTROL] Book " + bookId + " discarded (" + e.getMessage() + ").");
            control.addFailed(bookId);
            return false;
        }

        datalake.save(bookId, header, body);
        control.addDownloaded(bookId);

        System.out.println("[CONTROL] Book " + bookId + " successfully downloaded.");
        return true;
    }

    public boolean indexBook(int bookId) throws Exception {

        BookPaths paths = datalake.locate(bookId);

        if (paths == null) {
            System.out.println("[CONTROL] Book " + bookId + " not found in the datalake.");
            return false;
        }

        String header = Files.readString(paths.getHeader());
        String body = Files.readString(paths.getBody());

        metadata.save(bookId, header, paths.getBody());
        index.addBook(bookId, Tokenizer.tokenize(body));
        index.flush();
        control.addIndexed(bookId);

        System.out.println("[CONTROL] Book " + bookId + " successfully indexed.");
        return true;
    }

    public Set<Integer> pending() throws Exception {
        return control.pending();
    }

    /*
     * One step of the control layer:
     * 1. If there are downloaded books not yet indexed, index one.
     * 2. Otherwise, download a new book that has not been downloaded before.
     */
    public void step(List<Integer> candidates) throws Exception {

        Integer pendingBook = control.lowestPending();

        if (pendingBook != null) {
            System.out.println("[CONTROL] Scheduling book " + pendingBook + " for indexing...");
            indexBook(pendingBook);
            return;
        }

        Set<Integer> downloaded = control.downloaded();
        Set<Integer> failed = control.failed();

        if (candidates != null && !candidates.isEmpty()) {

            for (int bookId : candidates) {
                if (!downloaded.contains(bookId) && !failed.contains(bookId)) {
                    System.out.println("[CONTROL] Downloading new book with ID " + bookId + "...");
                    if (download(bookId)) {
                        return;
                    }
                }
            }

        } else {

            Random random = new Random();

            // Retry up to 10 random ids to find a new book
            for (int attempt = 0; attempt < 10; attempt++) {
                int bookId = random.nextInt(TOTAL_BOOKS) + 1;
                if (!downloaded.contains(bookId) && !failed.contains(bookId)) {
                    System.out.println("[CONTROL] Downloading new book with ID " + bookId + "...");
                    if (download(bookId)) {
                        return;
                    }
                }
            }
        }

        System.out.println("[CONTROL] No new books to download.");
    }

    public void run(int steps, List<Integer> candidates) throws Exception {
        for (int i = 0; i < steps; i++) {
            step(candidates);
        }
    }

    private String fetch(int bookId) throws Exception {

        if (sampleData != null) {
            Path file = sampleData.resolve("pg" + bookId + ".txt");
            if (!Files.exists(file)) {
                throw new IllegalArgumentException("book not found in " + sampleData);
            }
            return Files.readString(file);
        }

        return BookDownloader.downloadBook(bookId);
    }
}
