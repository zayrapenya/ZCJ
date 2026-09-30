/* Separar un libro de Gutenberg en header y body (equivale a split_book de Python). */
#ifndef BOOK_H
#define BOOK_H

typedef struct {
    char *header;   /* todo lo anterior al marcador START */
    char *body;     /* el texto del libro, sin la línea del marcador ni el footer */
} Book;

/* Separa el texto completo de un libro. Devuelve 0 si va bien, o -1 si no
   encuentra los marcadores START/END. Hay que liberar el resultado con free_book(). */
int split_book(const char *text, Book *out);

void free_book(Book *book);

#endif