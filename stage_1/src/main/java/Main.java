import metadata.MetadataStore;

public class Main {

    public static void main(String[] args) {

        try {

            MetadataStore metadataStore =
                    new MetadataStore("datamarts/metadata.db");

            System.out.println("MetadataStore creado correctamente.");

        } catch (Exception e) {

            e.printStackTrace();

        }
    }
}