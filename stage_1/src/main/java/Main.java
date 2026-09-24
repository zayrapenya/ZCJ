import datalake.BookManager;
import datalake.IndexStorage;
import datalake.InvertedIndex;
import datalake.SearchEngine;

import java.time.LocalDateTime;
import java.time.format.DateTimeFormatter;
import java.util.List;
import java.util.Map;

public class Main {

    public static void main(String[] args) {

        try {

            // ========================================
            // 1. LIBROS
            // ========================================

            int[] bookIds = {
                    1342,
                    84,
                    1661
            };


            // ========================================
            // 2. FECHA Y HORA DE ESTA EJECUCIÓN
            // ========================================

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


            // ========================================
            // 3. CREAR ÍNDICE GLOBAL
            // ========================================

            InvertedIndex index =
                    new InvertedIndex();


            // ========================================
            // 4. PROCESAR TODOS LOS LIBROS
            // ========================================

            for (int bookId : bookIds) {

                BookManager.processBook(
                        bookId,
                        index,
                        date,
                        hour
                );
            }


            // ========================================
            // 5. INFORMACIÓN DEL ÍNDICE
            // ========================================

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


            // ========================================
            // 6. GUARDAR ÍNDICE
            // ========================================

            IndexStorage.save(
                    index,
                    date,
                    hour
            );


            // ========================================
            // 7. CREAR BUSCADOR
            // ========================================

            SearchEngine searchEngine =
                    new SearchEngine(index);


            // ========================================
            // 8. PRUEBAS DEL BUSCADOR
            // ========================================

            System.out.println();
            System.out.println(
                    "================================"
            );

            System.out.println(
                    "PRUEBAS DEL BUSCADOR"
            );

            System.out.println(
                    "================================"
            );


            // Búsqueda de una palabra

            System.out.println();
            System.out.println(
                    "1. Búsqueda de palabra:"
            );

            System.out.println(
                    "darcy → "
                    + searchEngine.search(
                            "darcy"
                    )
            );


            // Búsqueda AND

            System.out.println();
            System.out.println(
                    "2. Búsqueda AND:"
            );

            System.out.println(
                    "darcy AND love → "
                    + searchEngine.searchAnd(
                            "darcy",
                            "love"
                    )
            );


            // Búsqueda de frase

            System.out.println();
            System.out.println(
                    "3. Búsqueda de frase:"
            );

            System.out.println(
                    "\"mr darcy\" → "
                    + searchEngine.searchPhrase(
                            "mr darcy"
                    )
            );


        } catch (Exception e) {

            e.printStackTrace();
        }
    }
}