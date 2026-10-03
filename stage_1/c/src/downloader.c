#include "downloader.h"

#include <curl/curl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "config.h"
#include "util.h"

typedef struct {
    char *data;
    size_t len;
} Download;

/* libcurl calls this for every received chunk */
static size_t on_data(char *chunk, size_t size, size_t count, void *ctx) {
    Download *dl = ctx;
    size_t bytes = size * count;
    char *bigger = realloc(dl->data, dl->len + bytes + 1);
    if (!bigger) return 0;  /* returning less than bytes aborts the transfer */
    dl->data = bigger;
    memcpy(dl->data + dl->len, chunk, bytes);
    dl->len += bytes;
    dl->data[dl->len] = '\0';
    return bytes;
}

FetchStatus download_book(int book_id, char **text) {
    static int initialized = 0;
    if (!initialized) {
        curl_global_init(CURL_GLOBAL_DEFAULT);
        initialized = 1;
    }

    char url[256];
    snprintf(url, sizeof(url), GUTENBERG_URL, book_id, book_id);

    CURL *curl = curl_easy_init();
    if (!curl) return FETCH_ERROR;

    Download dl = {NULL, 0};
    curl_easy_setopt(curl, CURLOPT_URL, url);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, on_data);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &dl);
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 30L);
    curl_easy_setopt(curl, CURLOPT_USERAGENT, "stage1-search-engine-c");

    CURLcode rc = curl_easy_perform(curl);
    long status = 0;
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &status);
    curl_easy_cleanup(curl);

    if (rc != CURLE_OK) {
        fprintf(stderr, "Download of book %d failed: %s\n", book_id, curl_easy_strerror(rc));
        free(dl.data);
        return FETCH_ERROR;
    }
    if (status == 404) {
        free(dl.data);
        return FETCH_NOT_FOUND;
    }
    if (status != 200 || !dl.data) {
        fprintf(stderr, "Download of book %d failed: HTTP %ld\n", book_id, status);
        free(dl.data);
        return FETCH_ERROR;
    }
    *text = dl.data;
    return FETCH_OK;
}

FetchStatus fetch_from_sample(int book_id, char **text) {
    char path[256];
    snprintf(path, sizeof(path), "%s/pg%d.txt", SAMPLE_DIR, book_id);
    *text = read_file(path, NULL);
    return *text ? FETCH_OK : FETCH_NOT_FOUND;
}