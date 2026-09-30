/* Punto de entrada. De momento solo comprueba que encuentra el sample data. */

#include <stdio.h>
#include <stdlib.h>

#include "config.h"
#include "util.h"

static const int SAMPLE_IDS[] = {11, 84, 98, 1342, 1661};
#define N_SAMPLE (sizeof(SAMPLE_IDS) / sizeof(SAMPLE_IDS[0]))

int main(void) {
    printf("Stage 1 - implementacion en C\n");
    printf("Comprobando el sample data en %s\n\n", SAMPLE_DIR);

    int found = 0;
    double start = now_seconds();

    for (size_t i = 0; i < N_SAMPLE; i++) {
        char path[256];
        snprintf(path, sizeof(path), "%s/pg%d.txt", SAMPLE_DIR, SAMPLE_IDS[i]);

        size_t size = 0;
        char *text = read_file(path, &size);
        if (!text) {
            printf("  %-28s NO ENCONTRADO\n", path);
            continue;
        }
        printf("  %-28s %8zu bytes\n", path, size);
        free(text);
        found++;
    }

    printf("\n%d de %zu libros leidos en %.3f ms\n",
           found, N_SAMPLE, 1000 * (now_seconds() - start));
    return found == (int)N_SAMPLE ? 0 : 1;
}