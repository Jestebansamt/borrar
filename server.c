#include "utils.h"

// --------------------------------------------
// Realiza la búsqueda según los criterios recibidos
// --------------------------------------------
void search(SearchCriteria criteria, Song *results, int *out_found) {
    FILE *f_index = fopen("hash_index.bin", "rb");
    FILE *f_nodes = fopen("index_nodes.bin", "rb");
    FILE *csv = fopen("songs.csv", "r");

    if (!f_index || !f_nodes || !csv) {
        perror("Error opening files");
        return;
    }

    // Calcular hash del título (clave principal)
    unsigned long hash = djb2_hash(criteria.titulo) % TABLE_SIZE;
    long bucket_offset = hash * sizeof(long);
    long current_offset;

    // Obtener nodo inicial del bucket
    fseek(f_index, bucket_offset, SEEK_SET);
    if (fread(&current_offset, sizeof(long), 1, f_index) != 1) {
        perror("Error reading index file");
        fclose(f_index);
        fclose(f_nodes);
        fclose(csv);
        return;
    }

    char *buffer = malloc(BUFFER_SIZE * sizeof(char));
    if (!buffer) {
        perror("Error allocating memory");
        return;
    }

    int found = 0;
    to_lower(criteria.titulo);
    to_lower(criteria.artist);

    // Recorrer lista enlazada del bucket
    while (current_offset != -1) {
        IndexNode node;
        fseek(f_nodes, current_offset, SEEK_SET);

        if (fread(&node, sizeof(IndexNode), 1, f_nodes) != 1) {
            perror("Error reading index node");
            break;
        }

        char node_key_lower[MAX_TITLE_SIZE];
        strncpy(node_key_lower, node.key, MAX_TITLE_SIZE);
        to_lower(node_key_lower);

        // Verificar si la clave coincide con el título
        if (strncmp(node_key_lower, criteria.titulo, MAX_TITLE_SIZE) == 0) {
            // Leer registro del CSV
            fseek(csv, node.data_offset, SEEK_SET);
            if (fgets(buffer, BUFFER_SIZE, csv)) {
                Song song = parse_song(buffer);

                char artist_lower[MAX_ARTIST_SIZE];
                strncpy(artist_lower, song.artist, MAX_ARTIST_SIZE);
                to_lower(artist_lower);

                int matches = 1;

                // Filtrar por artista si fue ingresado
                if (strlen(criteria.artist) > 0) {
                    matches = matches && (strcmp(artist_lower, criteria.artist) == 0);
                }

                if (matches) {
                    results[found++] = song;
                    if (found >= MAX_RESULTS) break; // límite de resultados
                }
            }
        }
        current_offset = node.next_offset;
    }

    *out_found = found;

    free(buffer);
    fclose(f_index);
    fclose(f_nodes);
    fclose(csv);
}

// --------------------------------------------
// Proceso principal del servidor
// --------------------------------------------
int main() {
    printf("========================================\n");
    printf("      MUSIC SEARCH SERVER ACTIVE\n");
    printf("========================================\n");

    // Crear FIFOs si no existen
    mkfifo(FIFO_C2S, 0666);
    mkfifo(FIFO_S2C, 0666);

    while (1) {
        Song *results = malloc(MAX_RESULTS * sizeof(Song));
        if (!results) {
            perror("Error allocating memory");
            return 1;
        }
        int found;

        // Esperar solicitud del cliente
        int rfd = open(FIFO_C2S, O_RDONLY);
        if (rfd == -1) {
            perror("Error opening FIFO_C2S");
            exit(EXIT_FAILURE);
        }

        SearchCriteria criteria;
        if (read(rfd, &criteria, sizeof(SearchCriteria)) == -1) {
            perror("Error reading FIFO_C2S");
            close(rfd);
            exit(EXIT_FAILURE);
        }
        close(rfd);

        // Ejecutar búsqueda
        search(criteria, results, &found);

        // Responder al cliente
        int wfd = open(FIFO_S2C, O_WRONLY);
        if (wfd == -1) {
            perror("Error opening FIFO_S2C");
            exit(EXIT_FAILURE);
        }

        if (write(wfd, &found, sizeof(int)) == -1) {
            perror("Error writing FIFO_S2C");
            close(wfd);
            exit(EXIT_FAILURE);
        }

        if (found == 0) {
            printf("========================================\n");
            printf("  No se encontraron coincidencias.\n");
            printf("========================================\n");
            close(wfd);
        } else {
            printf("========================================\n");
            printf("  %d canciones encontradas.\n", found);
            printf("========================================\n");
            for (int i = 0; i < found; i++) {
                Song *s = &results[i];

                if (write(wfd, s, sizeof(Song)) == -1) {
                    perror("Error writing FIFO_S2C");
                    close(wfd);
                    exit(EXIT_FAILURE);
                }

                // Log para ver resultados en consola del servidor
                printf("[%d] %s - %s (%d) | Views: %d\n",
                       i + 1, s->titulo, s->artist, s->year, s->views);
            }
            close(wfd);
        }
        free(results);
    }

    return 0;
}
