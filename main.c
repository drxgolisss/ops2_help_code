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

int set_handler(void (*f)(int), int sig)
{
    struct sigaction act = {0};
    act.sa_handler = f;
    if (sigaction(sig, &act, NULL) == -1)
        return -1;
    return 0;
}

void msleep(int millisec)
{
    struct timespec tt;
    tt.tv_sec = millisec / 1000;
    tt.tv_nsec = (millisec % 1000) * 1000000;
    while (nanosleep(&tt, &tt) == -1)
    {
    }
}

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

soldier_t* load_file(const char *path, int* soldier_num /*out parameter*/)
{
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

void print_knight_info(soldier_t id, const char* side)
{
    if (strcmp(side, SARACENI_FILE) == 0) {
        printf("I am Spanish knight %s. I will serve my king with my %d HP and %d attack.\n",
               id.name, id.health, id.attack);
    } else if (strcmp(side, FRANCI_FILE) == 0) {
        printf("I am Frankish knight %s. I will serve my king with my %d HP and %d attack\n",
               id.name, id.health, id.attack);
    }
}

void child_work(soldier_t id, int reading_fd, int* enemy_pipes, int enemy_num, const char* side)
{
    print_knight_info(id, side);
    srand(getpid());

    int flag = fcntl(reading_fd, F_GETFL);
    if (flag == -1)
        ERR("fcntl");
    if (fcntl(reading_fd, F_SETFL, flag | O_NONBLOCK) == -1)
        ERR("fcntl");

    while (id.health > 0) {
        int damage;
        while (1) {
            int ret = read(reading_fd, &damage, sizeof(damage));
            if (ret == -1) {
                if (errno == EAGAIN || errno == EWOULDBLOCK)
                    break;
                ERR("read");
            }
            if (ret == 0)
                break;
            if (ret > 0)
                id.health -= damage;
        }

        if (id.health <= 0)
            break;

ATTACK:
        int enemy = rand() % enemy_num;
        int attack = rand() % (id.attack + 1);

        if (write(enemy_pipes[2 * enemy + 1], &attack, sizeof(attack)) == -1) {
            if (errno == EPIPE) {
                goto ATTACK;
            }
            ERR("write");
        }

        if (attack == 0)
            printf("%s attacks his enemy, however he deflected\n", id.name);
        else if (attack <= 5)
            printf("%s goes to strike, he hit right and well\n", id.name);
        else
            printf("%s strikes powerful blow, the shield he breaks and inflicts a big wound\n", id.name);

        int sleep_time = rand() % 10 + 1;
        msleep(sleep_time);
    }

    printf("%s died\n", id.name);
}

void create_processes(soldier_t* soldiers, int soldiers_num, int* soldier_pipes,
                      int* enemy_pipes, int enemy_num, soldier_t* enemies, const char* side)
{
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

            child_work(soldiers[i], soldier_pipes[2 * i], enemy_pipes, enemy_num, side);

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
    if (set_handler(SIG_IGN, SIGPIPE))
        ERR("set_handler");

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
