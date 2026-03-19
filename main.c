// #define _POSIX_C_SOURCE 200809L
// #include <dirent.h>
// #include <errno.h>
// #include <fcntl.h>
// #include <limits.h>
// #include <signal.h>
// #include <stdint.h>
// #include <stdio.h>
// #include <stdlib.h>
// #include <string.h>
// #include <sys/stat.h>
// #include <sys/wait.h>
// #include <time.h>
// #include <unistd.h>

// #define SARACENI_FILE "saraceni.txt"
// #define FRANCI_FILE "franci.txt"

// #define ERR(source) \
//     (fprintf(stderr, "%s:%d\n", __FILE__, __LINE__), perror(source), kill(0, SIGKILL), exit(EXIT_FAILURE))

// #define MAX_SOLDIER_NAME 20

// typedef struct {
//     char name[MAX_SOLDIER_NAME + 1];
//     int health;
//     int attack;
// } soldier_t;

// int set_handler(void (*f)(int), int sig)
// {
//     struct sigaction act = {0};
//     act.sa_handler = f;
//     if (sigaction(sig, &act, NULL) == -1)
//         return -1;
//     return 0;
// }

// void msleep(int millisec)
// {
//     struct timespec tt;
//     tt.tv_sec = millisec / 1000;
//     tt.tv_nsec = (millisec % 1000) * 1000000;
//     while (nanosleep(&tt, &tt) == -1)
//     {
//     }
// }

// int count_descriptors()
// {
//     int count = 0;
//     DIR* dir;
//     struct dirent* entry;
//     struct stat stats;
//     if ((dir = opendir("/proc/self/fd")) == NULL)
//         ERR("opendir");
//     char path[PATH_MAX];
//     getcwd(path, PATH_MAX);
//     chdir("/proc/self/fd");
//     do
//     {
//         errno = 0;
//         if ((entry = readdir(dir)) != NULL)
//         {
//             if (lstat(entry->d_name, &stats))
//                 ERR("lstat");
//             if (!S_ISDIR(stats.st_mode))
//                 count++;
//         }
//     } while (entry != NULL);
//     if (chdir(path))
//         ERR("chdir");
//     if (closedir(dir))
//         ERR("closedir");
//     return count - 1;
// }

// soldier_t* load_file(const char *path, int* soldier_num /*out parameter*/)
// {
//     FILE* file = fopen(path, "r");
//     if (file == NULL) {
//         if (strcmp(path, FRANCI_FILE) == 0)
//             printf("Franks have not arrived on the battlefield\n");
//         else if (strcmp(path, SARACENI_FILE) == 0)
//             printf("Saracens have not arrived on the battlefield\n");
//         return NULL;
//     }

//     fscanf(file, "%d", soldier_num);
//     soldier_t* soldiers = malloc(*soldier_num * sizeof(soldier_t));
//     if (soldiers == NULL)
//         ERR("malloc");

//     for (int i = 0; i < *soldier_num; i++) {
//         fscanf(file, "%20s %d %d", soldiers[i].name, &soldiers[i].health, &soldiers[i].attack);
//     }

//     fclose(file);
//     return soldiers;
// }

// void print_knight_info(soldier_t id, const char* side)
// {
//     if (strcmp(side, SARACENI_FILE) == 0) {
//         printf("I am Spanish knight %s. I will serve my king with my %d HP and %d attack.\n",
//                id.name, id.health, id.attack);
//     } else if (strcmp(side, FRANCI_FILE) == 0) {
//         printf("I am Frankish knight %s. I will serve my king with my %d HP and %d attack\n",
//                id.name, id.health, id.attack);
//     }
// }

// void swap_enemy(int* enemy_pipes, int i, int p)
// {
//     int tmp_read = enemy_pipes[2 * i];
//     int tmp_write = enemy_pipes[2 * i + 1];

//     enemy_pipes[2 * i] = enemy_pipes[2 * p];
//     enemy_pipes[2 * i + 1] = enemy_pipes[2 * p + 1];

//     enemy_pipes[2 * p] = tmp_read;
//     enemy_pipes[2 * p + 1] = tmp_write;
// }

// void child_work(soldier_t id, int reading_fd, int* enemy_pipes, int enemy_num, const char* side)
// {
//     print_knight_info(id, side);
//     srand(getpid());

//     int flag = fcntl(reading_fd, F_GETFL);
//     if (flag == -1)
//         ERR("fcntl");
//     if (fcntl(reading_fd, F_SETFL, flag | O_NONBLOCK) == -1)
//         ERR("fcntl");

//     int p = enemy_num - 1;

//     while (id.health > 0) {
//         int damage;
//         while (1) {
//             int ret = read(reading_fd, &damage, sizeof(damage));
//             if (ret == -1) {
//                 if (errno == EAGAIN || errno == EWOULDBLOCK)
//                     break;
//                 ERR("read");
//             }
//             if (ret == 0)
//                 break;
//             if (ret > 0)
//                 id.health -= damage;
//         }

//         if (id.health < 0)
//             break;

//         if (p < 0)
//             break;

// ATTACK:
//         if (p < 0)
//             break;

//         int enemy = rand() % (p + 1);
//         int attack = rand() % (id.attack + 1);

//         if (write(enemy_pipes[2 * enemy + 1], &attack, sizeof(attack)) == -1) {
//             if (errno == EPIPE) {
//                 swap_enemy(enemy_pipes, enemy, p);
//                 p--;
//                 goto ATTACK;
//             }
//             ERR("write");
//         }

//         if (attack == 0)
//             printf("%s attacks his enemy, however he deflected\n", id.name);
//         else if (attack <= 5)
//             printf("%s goes to strike, he hit right and well\n", id.name);
//         else
//             printf("%s strikes powerful blow, the shield he breaks and inflicts a big wound\n", id.name);

//         int sleep_time = rand() % 10 + 1;
//         msleep(sleep_time);
//     }

//     if (id.health < 0)
//         printf("%s dies glorious death\n", id.name);
// }

// void create_processes(soldier_t* soldiers, int soldiers_num, int* soldier_pipes,
//                       int* enemy_pipes, int enemy_num, soldier_t* enemies, const char* side)
// {
//     for (int i = 0; i < soldiers_num; i++) {
//         pid_t pid = fork();
//         if (pid < 0)
//             ERR("fork");

//         if (pid == 0)
//         {
//             for (int j = 0; j < soldiers_num; j++) {
//                 close(soldier_pipes[2 * j + 1]);
//                 if (i != j)
//                     close(soldier_pipes[2 * j]);
//             }

//             for (int j = 0; j < enemy_num; j++) {
//                 close(enemy_pipes[2 * j]);
//             }

//             child_work(soldiers[i], soldier_pipes[2 * i], enemy_pipes, enemy_num, side);

//             close(soldier_pipes[2 * i]);
//             for (int j = 0; j < enemy_num; j++) {
//                 close(enemy_pipes[2 * j + 1]);
//             }

//             free(soldiers);
//             free(soldier_pipes);
//             free(enemy_pipes);
//             free(enemies);
//             exit(EXIT_SUCCESS);
//         }
//     }
// }

// int main()
// {
//     if (set_handler(SIG_IGN, SIGPIPE))
//         ERR("set_handler");

//     int franci_num, saraceni_num;

//     soldier_t* franci = load_file(FRANCI_FILE, &franci_num);
//     if (franci == NULL)
//         return EXIT_FAILURE;

//     soldier_t* saraceni = load_file(SARACENI_FILE, &saraceni_num);
//     if (saraceni == NULL) {
//         free(franci);
//         return EXIT_FAILURE;
//     }

//     int *franci_pipes, *saraceni_pipes;
//     franci_pipes = malloc(2 * franci_num * sizeof(int));
//     saraceni_pipes = malloc(2 * saraceni_num * sizeof(int));
//     if (franci_pipes == NULL || saraceni_pipes == NULL)
//         ERR("malloc");

//     for (int i = 0; i < franci_num; i++) {
//         if (pipe(&franci_pipes[i * 2]))
//             ERR("pipe");
//     }

//     for (int i = 0; i < saraceni_num; i++) {
//         if (pipe(&saraceni_pipes[i * 2]))
//             ERR("pipe");
//     }

//     create_processes(franci, franci_num, franci_pipes, saraceni_pipes, saraceni_num, saraceni, FRANCI_FILE);
//     create_processes(saraceni, saraceni_num, saraceni_pipes, franci_pipes, franci_num, franci, SARACENI_FILE);

//     for (int i = 0; i < franci_num; i++) {
//         close(franci_pipes[2 * i]);
//         close(franci_pipes[2 * i + 1]);
//     }

//     for (int i = 0; i < saraceni_num; i++) {
//         close(saraceni_pipes[2 * i]);
//         close(saraceni_pipes[2 * i + 1]);
//     }

//     free(franci_pipes);
//     free(saraceni_pipes);
//     free(franci);
//     free(saraceni);

//     while (wait(NULL) > 0)
//         ;

//     return EXIT_SUCCESS;
// }






          // lab from consultation


//stage 1


// #define _POSIX_C_SOURCE 200809L

// #include <errno.h>
// #include <signal.h>
// #include <stdio.h>
// #include <stdlib.h>
// #include <sys/wait.h>
// #include <unistd.h>

// #define ERR(source) \
//     (fprintf(stderr, "%s:%d\n", __FILE__, __LINE__), perror(source), kill(0, SIGKILL), exit(EXIT_FAILURE))

// #define MAX_GRAPH_NODES 32
// #define MAX_PATH_LENGTH (2 * MAX_GRAPH_NODES)

// typedef struct node
// {
//     int indexes[MAX_GRAPH_NODES];
//     int neighbours_num;
//     int pipe[2];
// } node_t;

// typedef struct graph
// {
//     node_t nodes[MAX_GRAPH_NODES];
//     int node_num;
// } graph_t;

// void usage(int argc, char* argv[])
// {
//     printf("%s graph start dest\n", argv[0]);
//     printf("  graph - path to file containing colony graph\n");
//     printf("  start - starting node index\n");
//     printf("  dest - destination node index\n");
//     exit(EXIT_FAILURE);
// }

// graph_t read_colony(char* name)
// {
//     FILE* fl = fopen(name, "r");
//     if (fl == NULL) {
//         ERR("fopen");
//     }

//     graph_t graph = {0};

//     if (fscanf(fl, "%d", &graph.node_num) != 1) {
//         ERR("fscanf");
//     }

//     while (1) {
//         int from;
//         int to;
//         if (fscanf(fl, "%d %d", &from, &to) != 2) {
//             break;
//         }
//         node_t* node_ptr = &graph.nodes[from];
//         node_ptr->indexes[node_ptr->neighbours_num++] = to;
//     }

//     fclose(fl);
//     return graph;
// }

// void child_work(node_t node, int index)
// {
//     printf("{%d}: ", index);
//     for (int i = 0; i < node.neighbours_num; i++) {
//         printf("%d ", node.indexes[i]);
//     }
//     printf("\n");
// }

// int main(int argc, char* argv[])
// {
//     if (argc != 4)
//         usage(argc, argv);

//     graph_t graph = read_colony(argv[1]);

//     for (int i = 0; i < graph.node_num; i++) {
//         pid_t pid = fork();
//         if (pid == -1) {
//             ERR("fork");
//         }
//         else if (pid == 0) {
//             child_work(graph.nodes[i], i);
//             exit(EXIT_SUCCESS);
//         }
//     }

//     while (wait(NULL) > 0) {}

//     exit(EXIT_SUCCESS);
// }




// stage 2





// #define _POSIX_C_SOURCE 200809L

// #include <errno.h>
// #include <signal.h>
// #include <stdio.h>
// #include <stdlib.h>
// #include <sys/wait.h>
// #include <unistd.h>

// #define ERR(source) \
//     (fprintf(stderr, "%s:%d\n", __FILE__, __LINE__), perror(source), kill(0, SIGKILL), exit(EXIT_FAILURE))

// #define MAX_GRAPH_NODES 32
// #define MAX_PATH_LENGTH (2 * MAX_GRAPH_NODES)

// volatile sig_atomic_t stop_work;
// static int read_fd = -1;

// typedef struct node
// {
//     int indexes[MAX_GRAPH_NODES];
//     int neighbours_num;
//     int pipe[2];
// } node_t;

// typedef struct graph
// {
//     node_t nodes[MAX_GRAPH_NODES];
//     int node_num;
// } graph_t;

// int set_handler(void (*f)(int), int sig)
// {
//     struct sigaction act = {0};
//     act.sa_handler = f;
//     if (sigaction(sig, &act, NULL) == -1)
//         return -1;
//     return 0;
// }

// void sig_handler(int signal)
// {
//     if (signal == SIGINT) {
//         stop_work = 1;
//         if (read_fd != -1)
//             close(read_fd);
//     }
// }

// void usage(int argc, char* argv[])
// {
//     printf("%s graph start dest\n", argv[0]);
//     printf("  graph - path to file containing colony graph\n");
//     printf("  start - starting node index\n");
//     printf("  dest - destination node index\n");
//     exit(EXIT_FAILURE);
// }

// graph_t read_colony(char* name)
// {
//     FILE* fl = fopen(name, "r");
//     if (fl == NULL) {
//         ERR("fopen");
//     }

//     graph_t graph = {0};

//     if (fscanf(fl, "%d", &graph.node_num) != 1) {
//         ERR("fscanf");
//     }

//     while (1) {
//         int from;
//         int to;
//         if (fscanf(fl, "%d %d", &from, &to) != 2) {
//             break;
//         }
//         node_t* node_ptr = &graph.nodes[from];
//         node_ptr->indexes[node_ptr->neighbours_num++] = to;
//     }

//     fclose(fl);
//     return graph;
// }

// void child_work(node_t node, int index, int fd_w[MAX_GRAPH_NODES])
// {
//     if (set_handler(sig_handler, SIGINT))
//         ERR("set_handler");

//     printf("{%d}: ", index);
//     for (int i = 0; i < node.neighbours_num; i++) {
//         printf("%d ", node.indexes[i]);
//     }
//     printf("\n");

//     read_fd = node.pipe[0];

//     while (!stop_work) {
//         char p;
//         if (read(node.pipe[0], &p, 1) < 0) {
//             if (errno == EINTR || errno == EBADF) {
//                 break;
//             }
//             ERR("read");
//         }
//     }
// }

// int main(int argc, char* argv[])
// {
//     if (set_handler(SIG_IGN, SIGINT))
//         ERR("set_handler");

//     if (argc != 4)
//         usage(argc, argv);

//     int start = atoi(argv[2]);
//     int dest = atoi(argv[3]);

//     graph_t graph = read_colony(argv[1]);

//     if (start < 0 || start >= graph.node_num || dest < 0 || dest >= graph.node_num)
//         usage(argc, argv);

//     for (int i = 0; i < graph.node_num; i++) {
//         if (pipe(graph.nodes[i].pipe) == -1) {
//             ERR("pipe");
//         }
//     }

//     for (int i = 0; i < graph.node_num; i++) {
//         pid_t pid = fork();
//         if (pid == -1) {
//             ERR("fork");
//         }
//         else if (pid == 0) {
//             int fd_w[MAX_GRAPH_NODES];

//             for (int j = 0; j < graph.node_num; j++) {
//                 if (i == j) {
//                     close(graph.nodes[j].pipe[1]);
//                 }
//                 else {
//                     close(graph.nodes[j].pipe[0]);
//                     int is_neighbour = 0;
//                     for (int k = 0; k < graph.nodes[i].neighbours_num; k++) {
//                         if (graph.nodes[i].indexes[k] == j) {
//                             is_neighbour = 1;
//                             break;
//                         }
//                     }
//                     if (is_neighbour) {
//                         fd_w[j] = graph.nodes[j].pipe[1];
//                     }
//                     else {
//                         close(graph.nodes[j].pipe[1]);
//                     }
//                 }
//             }

//             child_work(graph.nodes[i], i, fd_w);

//             close(graph.nodes[i].pipe[0]);
//             for (int j = 0; j < graph.nodes[i].neighbours_num; j++) {
//                 close(fd_w[graph.nodes[i].indexes[j]]);
//             }

//             exit(EXIT_SUCCESS);
//         }
//     }

//     for (int i = 0; i < graph.node_num; i++) {
//         close(graph.nodes[i].pipe[0]);
//         if (i != start) {
//             close(graph.nodes[i].pipe[1]);
//         }
//     }

//     pause();

//     close(graph.nodes[start].pipe[1]);

//     while (wait(NULL) > 0) {}

//     exit(EXIT_SUCCESS);
// }
