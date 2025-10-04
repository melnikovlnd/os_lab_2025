#include <ctype.h>
#include <limits.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <sys/time.h>
#include <sys/types.h>
#include <sys/wait.h>

#include <getopt.h>

#include "find_min_max.h"
#include "utils.h"

int main(int argc, char **argv) {
    int seed = -1;
    int array_size = -1;
    int pnum = -1;
    bool with_files = false;

    while (true) {
        static struct option options[] = {
            {"seed", required_argument, 0, 0},
            {"array_size", required_argument, 0, 0},
            {"pnum", required_argument, 0, 0},
            {"by_files", no_argument, 0, 'f'},
            {0, 0, 0, 0}
        };

        int option_index = 0;
        int c = getopt_long(argc, argv, "f", options, &option_index);

        if (c == -1) break;

        switch (c) {
            case 0:
                switch (option_index) {
                    case 0:
                        seed = atoi(optarg);
                        if (seed <= 0) {
                            printf("seed must be a positive number\n");
                            return 1;
                        }
                        break;
                    case 1:
                        array_size = atoi(optarg);
                        if (array_size <= 0) {
                            printf("array_size must be a positive number\n");
                            return 1;
                        }
                        break;
                    case 2:
                        pnum = atoi(optarg);
                        if (pnum <= 0) {
                            printf("pnum must be a positive number\n");
                            return 1;
                        }
                        break;
                    case 3:
                        with_files = true;
                        break;
                    default:
                        printf("Index %d is out of options\n", option_index);
                        return 1;
                }
                break;
            case 'f':
                with_files = true;
                break;
            case '?':
                return 1;
            default:
                printf("getopt returned character code 0%o?\n", c);
                return 1;
        }
    }

    if (optind < argc) {
        printf("Has at least one no option argument\n");
        return 1;
    }

    if (seed == -1 || array_size == -1 || pnum == -1) {
        printf("Usage: %s --seed \"num\" --array_size \"num\" --pnum \"num\" [--by_files]\n",
               argv[0]);
        return 1;
    }

    int *array = malloc(sizeof(int) * array_size);
    if (!array) {
        perror("malloc");
        return 1;
    }
    GenerateArray(array, array_size, seed);

    int *pipefd = NULL;
    if (!with_files) {
        pipefd = malloc(2 * pnum * sizeof(int));
        if (!pipefd) {
            perror("malloc pipefd");
            free(array);
            return 1;
        }
    }

    int active_child_processes = 0;
    struct timeval start_time;
    gettimeofday(&start_time, NULL);

    for (int i = 0; i < pnum; i++) {
        int chunk_size = array_size / pnum;
        int begin = i * chunk_size;
        int end = (i == pnum - 1) ? array_size : begin + chunk_size;

        if (!with_files) {
            if (pipe(pipefd + 2 * i) == -1) {
                perror("pipe");
                free(array);
                free(pipefd);
                return 1;
            }
        }

        pid_t child_pid = fork();
        if (child_pid < 0) {
            perror("fork");
            free(array);
            if (!with_files) free(pipefd);
            return 1;
        }

        if (child_pid == 0) {
            struct MinMax mm = GetMinMax(array, begin, end);

            if (with_files) {
                char filename[256];
                snprintf(filename, sizeof(filename), "minmax_%d.txt", i);
                FILE *f = fopen(filename, "w");
                if (!f) {
                    perror("fopen (child)");
                    exit(1);
                }
                fprintf(f, "%d %d", mm.min, mm.max);
                fclose(f);
            } else {

                close(pipefd[2 * i]);
                if (write(pipefd[2 * i + 1], &mm, sizeof(mm)) != sizeof(mm)) {
                    perror("write");
                    exit(1);
                }
                close(pipefd[2 * i + 1]);
            }
            exit(0);
        } else {
            active_child_processes++;
        }
    }

    while (active_child_processes > 0) {
        if (wait(NULL) == -1) {
            perror("wait");
            break;
        }
        active_child_processes--;
    }

    struct MinMax global_min_max;
    global_min_max.min = INT_MAX;
    global_min_max.max = INT_MIN;

    for (int i = 0; i < pnum; i++) {
        int min = INT_MAX;
        int max = INT_MIN;

        if (with_files) {
            char filename[256];
            snprintf(filename, sizeof(filename), "minmax_%d.txt", i);
            FILE *f = fopen(filename, "r");
            if (!f) {
                perror("fopen (parent)");
                free(array);
                if (!with_files) free(pipefd);
                return 1;
            }
            if (fscanf(f, "%d %d", &min, &max) != 2) {
                fprintf(stderr, "Failed to read min/max from %s\n", filename);
                fclose(f);
                unlink(filename);
                free(array);
                if (!with_files) free(pipefd);
                return 1;
            }
            fclose(f);
            unlink(filename); 
        } else {
            close(pipefd[2 * i + 1]);

            struct MinMax mm;
            ssize_t n = read(pipefd[2 * i], &mm, sizeof(mm));
            if (n == -1) {
                perror("read");
                free(array);
                free(pipefd);
                return 1;
            } else if (n != sizeof(mm)) {
                fprintf(stderr, "read: expected %zu bytes, got %zd\n", sizeof(mm), n);
                free(array);
                free(pipefd);
                return 1;
            }
            close(pipefd[2 * i]);
            min = mm.min;
            max = mm.max;
        }

        if (min < global_min_max.min) global_min_max.min = min;
        if (max > global_min_max.max) global_min_max.max = max;
    }

    struct timeval finish_time;
    gettimeofday(&finish_time, NULL);

    double elapsed_time = (finish_time.tv_sec - start_time.tv_sec) * 1000.0;
    elapsed_time += (finish_time.tv_usec - start_time.tv_usec) / 1000.0;

    free(array);
    if (!with_files) {
        free(pipefd);
    }

    printf("Min: %d\n", global_min_max.min);
    printf("Max: %d\n", global_min_max.max);
    printf("Elapsed time: %fms\n", elapsed_time);
    fflush(NULL);
    return 0;
}