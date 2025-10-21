#ifndef UTILS_H
#define UTILS_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

#define FIFO_C2S "/tmp/fifo_c2s"  // Cliente → Servidor
#define FIFO_S2C "/tmp/fifo_s2c"  // Servidor → Cliente

#define TABLE_SIZE 20000003       // Número primo grande
#define BUFFER_SIZE 1024

#define MAX_TITLE_SIZE 128
#define MAX_ARTIST_SIZE 128
#define MAX_TAG_SIZE 64
#define MAX_FEATURES_SIZE 128
#define MAX_LYRICS_SIZE 512
#define MAX_LANG_SIZE 16
#define MAX_RESULTS 128           // para límite de búsqueda

// ---------------------------
// 🔹 ESTRUCTURAS DE DATOS
// ---------------------------

typedef struct {
    char titulo[MAX_TITLE_SIZE];
    char tag[MAX_TAG_SIZE];
    char artist[MAX_ARTIST_SIZE];
    int year;
    int views;
    char features[MAX_FEATURES_SIZE];
    char lyrics[MAX_LYRICS_SIZE];
    int id;
    char language_cld3[MAX_LANG_SIZE];
    char language_ft[MAX_LANG_SIZE];
    char language[MAX_LANG_SIZE];
} Song;

typedef struct {
    char titulo[MAX_TITLE_SIZE];
    char artist[MAX_ARTIST_SIZE];
} SearchCriteria;

typedef struct {
    char key[MAX_TITLE_SIZE];
    long data_offset;
    long next_offset;
} IndexNode;

// ---------------------------
// 🔹 FUNCIONES AUXILIARES
// ---------------------------

static inline unsigned long djb2_hash(const char *str) {
    unsigned long hash = 5381;
    int c;
    while ((c = *str++))
        hash = ((hash << 5) + hash) + c;
    return hash;
}

static inline void to_lower(char *str) {
    for (; *str; ++str)
        *str = tolower((unsigned char)*str);
}

static inline Song parse_song(const char *line) {
    Song song;
    char *buffer = strdup(line);
    char *token = strtok(buffer, ",");

    // 1. titulo
    strncpy(song.titulo, token ? token : "", MAX_TITLE_SIZE - 1);
    song.titulo[MAX_TITLE_SIZE - 1] = '\0';

    // 2. tag
    token = strtok(NULL, ",");
    strncpy(song.tag, token ? token : "", MAX_TAG_SIZE - 1);

    // 3. artist
    token = strtok(NULL, ",");
    strncpy(song.artist, token ? token : "", MAX_ARTIST_SIZE - 1);
    song.artist[MAX_ARTIST_SIZE - 1] = '\0';

    // 4. year
    token = strtok(NULL, ",");
    song.year = token ? atoi(token) : 0;

    // 5. views
    token = strtok(NULL, ",");
    song.views = token ? atoi(token) : 0;

    // 6. features
    token = strtok(NULL, ",");
    strncpy(song.features, token ? token : "", MAX_FEATURES_SIZE - 1);

    // 7. lyrics
    token = strtok(NULL, ",");
    strncpy(song.lyrics, token ? token : "", MAX_LYRICS_SIZE - 1);

    // 8. id
    token = strtok(NULL, ",");
    song.id = token ? atoi(token) : 0;

    // 9–11. idiomas
    token = strtok(NULL, ",");
    strncpy(song.language_cld3, token ? token : "", MAX_LANG_SIZE - 1);
    token = strtok(NULL, ",");
    strncpy(song.language_ft, token ? token : "", MAX_LANG_SIZE - 1);
    token = strtok(NULL, ",\n");
    strncpy(song.language, token ? token : "", MAX_LANG_SIZE - 1);
    song.language[MAX_LANG_SIZE - 1] = '\0';
    trim_newline(song.language);
    to_lower(song.language);

    free(buffer);
    return song;
}

#endif // UTILS_H
