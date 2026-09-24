import datalake.BookManager;
import datalake.IndexStorage;
import datalake.InvertedIndex;

import java.time.LocalDateTime;
import java.time.format.DateTimeFormatter;
import java.util.List;
import java.util.Map;

public class Main {

    public static void main(String[] args) {

        try {

            // Libros que vamos a utilizar
            int[] bookIds = {
                    1342,
                    84,
                    1661
            };

            // Índice global
            InvertedIndex index =
                    new InvertedIndex();

            // Procesar todos los libros
            for (int bookId : bookIds) {

                BookManager.processBook(
                        bookId,
                        index
                );
            }

            // Mostrar información del índice
            System.out.println();
            System.out.println(
                    "================================"
            );

            System.out.println(
                    "ÍNDICE GLOBAL"
            );

            System.out.println(
                    "================================"
            );

            System.out.println(
                    "Número de palabras diferentes: "
                    + index.size()
            );

            // Obtener fecha y hora
            LocalDateTime now =
                    LocalDateTime.now();

            String date =
                    now.format(
                            DateTimeFormatter.ofPattern(
                                    "yyyyMMdd"
                            )
                    );

            String hour =
                    now.format(
                            DateTimeFormatter.ofPattern(
                                    "HH"
                            )
                    );

            // Guardar índice
            IndexStorage.save(
                    index,
                    date,
                    hour
            );

            // Pruebas de búsqueda
            System.out.println();
            System.out.println(
                    "================================"
            );

            System.out.println(
                    "PRUEBAS DE BÚSQUEDA"
            );

            System.out.println(
                    "================================"
            );

            String[] wordsToSearch = {
                    "darcy",
                    "elizabeth",
                    "love",
                    "holmes",
                    "alice"
            };

            for (String word : wordsToSearch) {

                Map<Integer, List<Integer>> documents =
                        index.search(word);

                System.out.println(
                        word + " → " + documents
                );
            }
            System.out.println();
System.out.println(
        "================================"
);

System.out.println(
        "BÚSQUEDA AND"
);

System.out.println(
        "================================"
);

Map<Integer, List<Integer>> andResult =
        index.searchAnd(
                "darcy",
                "love"
        );

System.out.println(
        "darcy AND love → "
        + andResult
);


System.out.println();
System.out.println(
        "================================"
);

System.out.println(
        "BÚSQUEDA DE FRASES"
);

System.out.println(
        "================================"
);

Map<Integer, List<Integer>> phraseResult =
        index.searchPhrase(
                "mr darcy"
        );

System.out.println(
        "\"mr darcy\" → "
        + phraseResult
);

        } catch (Exception e) {

            e.printStackTrace();

        }
    }
}