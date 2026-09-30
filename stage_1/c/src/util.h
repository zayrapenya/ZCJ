/* Funciones auxiliares: ficheros, carpetas y cronómetro. */
#ifndef UTIL_H
#define UTIL_H

#include <stddef.h>

/* Lee un fichero entero (modo binario). Devuelve texto terminado en '\0'
   que hay que liberar con free(), o NULL si no existe. */
char *read_file(const char *path, size_t *out_len);

/* Escribe (sobrescribe) un fichero. Devuelve 0 si va bien. */
int write_file(const char *path, const char *data, size_t len);

/* Añade texto al final de un fichero. Devuelve 0 si va bien. */
int append_file(const char *path, const char *text);

/* Crea una carpeta y sus intermedias (como "mkdir -p"). Devuelve 0 si va bien. */
int make_dirs(const char *path);

/* Devuelve 1 si la ruta existe. */
int path_exists(const char *path);

/* Tiempo actual en segundos con alta precisión (para benchmarks). */
double now_seconds(void);

#endif