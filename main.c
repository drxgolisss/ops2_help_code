#define _POSIX_C_SOURCE 200809L
#include <errno.h>
#include <limits.h>
#include <signal.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define SARACENI_FILE "saraceni.txt"
#define FRANCI_FILE "franci.txt"

#define ERR(source) \
    (fprintf(stderr, "%s:%d\n", __FILE__, __LINE__), perror(source), kill(0, SIGKILL), exit(EXIT_FAILURE))

#define MAX_SOLDIER_NAME 20

typedef struct {
    char name[MAX_SOLDIER_NAME + 1];
    int health;
    int attack;
} soldier_t;

soldier_t* load_file(const char *path, int* soldier_num /*out parameter*/) {
    FILE* file = fopen(path, "r");
    if (file == NULL) {
        if (strcmp(path, FRANCI_FILE) == 0) {
            printf("Franks have not arrived on the battlefield\n");
        } else if (strcmp(path, SARACENI_FILE) == 0) {
            printf("Saracens have not arrived on the battlefield\n");
        }
        return NULL;
    }

    fscanf(file, "%d", soldier_num);
    soldier_t* soldiers = malloc(*soldier_num * sizeof(soldier_t));
    if (soldiers == NULL)
        ERR("malloc");

    for (int i = 0; i < *soldier_num; i++) {
        fscanf(file, "%20s %d %d", soldiers[i].name, &soldiers[i].health, &soldiers[i].attack);

        if (strcmp(path, SARACENI_FILE) == 0) {
            printf("I am Spanish knight %s. I will serve my king with my %d HP and %d attack.\n",
                   soldiers[i].name, soldiers[i].health, soldiers[i].attack);
        } else if (strcmp(path, FRANCI_FILE) == 0) {
            printf("I am Frankish knight %s. I will serve my king with my %d HP and %d attack.\n",
                   soldiers[i].name, soldiers[i].health, soldiers[i].attack);
        }
    }

    fclose(file);
    return soldiers;
}

int main() {
    int franci_num, saraceni_num;

    soldier_t* franci = load_file(FRANCI_FILE, &franci_num);
    if (franci == NULL)
        return EXIT_FAILURE;

    soldier_t* saraceni = load_file(SARACENI_FILE, &saraceni_num);
    if (saraceni == NULL) {
        free(franci);
        return EXIT_FAILURE;
    }

    free(franci);
    free(saraceni);

    return EXIT_SUCCESS;
}
