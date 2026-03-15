#define _POSIX_C_SOURCE 200809L
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <signal.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <time.h>
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

int count_descriptors()
{
    int count = 0;
    DIR* dir;
    struct dirent* entry;
    struct stat stats;
    if ((dir = opendir("/proc/self/fd")) == NULL)
        ERR("opendir");
    char path[PATH_MAX];
    getcwd(path, PATH_MAX);
    chdir("/proc/self/fd");
    do
    {
        errno = 0;
        if ((entry = readdir(dir)) != NULL)
        {
            if (lstat(entry->d_name, &stats))
                ERR("lstat");
            if (!S_ISDIR(stats.st_mode))
                count++;
        }
    } while (entry != NULL);
    if (chdir(path))
        ERR("chdir");
    if (closedir(dir))
        ERR("closedir");
    return count - 1;
}

soldier_t* load_file(const char *path, int* soldier_num /*out parameter*/) {
    FILE* file = fopen(path, "r");
    if (file == NULL) {
        if (strcmp(path, FRANCI_FILE) == 0)
            printf("Franks have not arrived on the battlefield\n");
        else if (strcmp(path, SARACENI_FILE) == 0)
            printf("Saracens have not arrived on the battlefield\n");
        return NULL;
    }

    fscanf(file, "%d", soldier_num);
    soldier_t* soldiers = malloc(*soldier_num * sizeof(soldier_t));
    if (soldiers == NULL)
        ERR("malloc");

    for (int i = 0; i < *soldier_num; i++) {
        fscanf(file, "%20s %d %d", soldiers[i].name, &soldiers[i].health, &soldiers[i].attack);
    }

    fclose(file);
    return soldiers;
}

void child_work(soldier_t id, const char* side) {
    if (strcmp(side, SARACENI_FILE) == 0) {
        printf("I am Spanish knight %s. I will serve my king with my %d HP and %d attack.\n",
               id.name, id.health, id.attack);
    } else if (strcmp(side, FRANCI_FILE) == 0) {
        printf("I am Frankish knight %s. I will serve my king with my %d HP and %d attack\n",
               id.name, id.health, id.attack);
    }

    // printf("%s: %d\n", id.name, count_descriptors());
}

void create_processes(soldier_t* soldiers, int soldiers_num, int* soldier_pipes,
                      int* enemy_pipes, int enemy_num, soldier_t* enemies, const char* side) {
    for (int i = 0; i < soldiers_num; i++) {
        pid_t pid = fork();
        if (pid < 0)
            ERR("fork");

        if (pid == 0)
        {
            for (int j = 0; j < soldiers_num; j++) {
                close(soldier_pipes[2 * j + 1]);
                if (i != j)
                    close(soldier_pipes[2 * j]);
            }

            for (int j = 0; j < enemy_num; j++) {
                close(enemy_pipes[2 * j]);
            }

            child_work(soldiers[i], side);

            close(soldier_pipes[2 * i]);
            for (int j = 0; j < enemy_num; j++) {
                close(enemy_pipes[2 * j + 1]);
            }

            free(soldiers);
            free(soldier_pipes);
            free(enemy_pipes);
            free(enemies);
            exit(EXIT_SUCCESS);
        }
    }
}

int main()
{
    int franci_num, saraceni_num;

    soldier_t* franci = load_file(FRANCI_FILE, &franci_num);
    if (franci == NULL)
        return EXIT_FAILURE;

    soldier_t* saraceni = load_file(SARACENI_FILE, &saraceni_num);
    if (saraceni == NULL) {
        free(franci);
        return EXIT_FAILURE;
    }

    int *franci_pipes, *saraceni_pipes;
    franci_pipes = malloc(2 * franci_num * sizeof(int));
    saraceni_pipes = malloc(2 * saraceni_num * sizeof(int));
    if (franci_pipes == NULL || saraceni_pipes == NULL)
        ERR("malloc");

    for (int i = 0; i < franci_num; i++) {
        if (pipe(&franci_pipes[i * 2]))
            ERR("pipe");
    }

    for (int i = 0; i < saraceni_num; i++) {
        if (pipe(&saraceni_pipes[i * 2]))
            ERR("pipe");
    }

    create_processes(franci, franci_num, franci_pipes, saraceni_pipes, saraceni_num, saraceni, FRANCI_FILE);
    create_processes(saraceni, saraceni_num, saraceni_pipes, franci_pipes, franci_num, franci, SARACENI_FILE);

    for (int i = 0; i < franci_num; i++) {
        close(franci_pipes[2 * i]);
        close(franci_pipes[2 * i + 1]);
    }

    for (int i = 0; i < saraceni_num; i++) {
        close(saraceni_pipes[2 * i]);
        close(saraceni_pipes[2 * i + 1]);
    }

    free(franci_pipes);
    free(saraceni_pipes);
    free(franci);
    free(saraceni);

    while (wait(NULL) > 0)
        ;

    return EXIT_SUCCESS;
}
