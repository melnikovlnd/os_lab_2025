#include <ctype.h>
#include <limits.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <errno.h>

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
    int timeout = 0; // 0 means no timeout
    bool with_files = false;
    pid_t *child_pids = NULL;

    while (true) {
        int current_optind = optind ? optind : 1;

        static struct option options[] = {
            {"seed", required_argument, 0, 0},
            {"array_size", required_argument, 0, 0},
            {"pnum", required_argument, 0, 0},
            {"by_files", no_argument, 0, 'f'},
            {"timeout", required_argument, 0, 't'},
            {0, 0, 0, 0}
        };

        int option_index = 0;
        int c = getopt_long(argc, argv, "ft:", options, &option_index);

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
                }
                break;
            case 'f':
                with_files = true;
                break;
            case 't':
                timeout = atoi(optarg);
                if (timeout <= 0) {
                    printf("timeout must be a positive number\n");
                    return 1;
                }
                break;
            case '?':
                break;
            default:
                printf("getopt returned character code 0%o?\n", c);
        }
    }

    if (optind < argc) {
        printf("Has at least one no option argument\n");
        return 1;
    }

    if (seed == -1 || array_size == -1 || pnum == -1) {
        printf("Usage: %s --seed \"num\" --array_size \"num\" --pnum \"num\" [--timeout \"num\"]\n",
               argv[0]);
        return 1;
    }

    child_pids = malloc(sizeof(pid_t) * pnum);
    if (child_pids == NULL) {
        printf("Failed to allocate memory for child PIDs\n");
        return 1;
    }

    int *array = malloc(sizeof(int) * array_size);
    GenerateArray(array, array_size, seed);
    int active_child_processes = 0;

    struct timeval start_time;
    gettimeofday(&start_time, NULL);

    // Создаем pipes если нужно
    int **pipefd = NULL;
    if (!with_files) {
        pipefd = malloc(sizeof(int*) * pnum);
        for (int i = 0; i < pnum; i++) {
            pipefd[i] = malloc(sizeof(int) * 2);
            if (pipe(pipefd[i]) == -1) {
                printf("Pipe creation failed!\n");
                return 1;
            }
        }
    }

    // Запускаем дочерние процессы
    for (int i = 0; i < pnum; i++) {
        pid_t child_pid = fork();
        if (child_pid >= 0) {
            active_child_processes += 1;
            child_pids[i] = child_pid;
            
            if (child_pid == 0) {
                // child process
                int step = array_size / pnum;
                int start = i * step;
                int end = (i == pnum - 1) ? array_size : (i + 1) * step;
                
                struct MinMax local_min_max = GetMinMax(array, start, end);
                
                if (with_files) {
                    // Исправлено: увеличен размер буфера
                    char filename[32];
                    snprintf(filename, sizeof(filename), "min_max_%d.txt", i);
                    FILE *file = fopen(filename, "w");
                    if (file != NULL) {
                        fprintf(file, "%d %d", local_min_max.min, local_min_max.max);
                        fclose(file);
                    }
                } else {
                    close(pipefd[i][0]);
                    write(pipefd[i][1], &local_min_max.min, sizeof(int));
                    write(pipefd[i][1], &local_min_max.max, sizeof(int));
                    close(pipefd[i][1]);
                }
                return 0;
            }
        } else {
            printf("Fork failed!\n");
            return 1;
        }
    }

    // Ожидаем завершения процессов с таймаутом
    if (timeout > 0) {
        printf("Waiting for child processes with timeout %d seconds...\n", timeout);
        
        sleep(timeout);
        
        for (int i = 0; i < pnum; i++) {
            int status;
            pid_t result = waitpid(child_pids[i], &status, WNOHANG);
            
            if (result == 0) {
                printf("Process %d exceeded timeout, sending SIGKILL\n", child_pids[i]);
                kill(child_pids[i], SIGKILL);
                waitpid(child_pids[i], &status, 0);
                active_child_processes -= 1;
            } else if (result > 0) {
                active_child_processes -= 1;
            }
        }
    } else {
        while (active_child_processes > 0) {
            wait(NULL);
            active_child_processes -= 1;
        }
    }

    // Сбор результатов
    struct MinMax min_max;
    min_max.min = INT_MAX;
    min_max.max = INT_MIN;

    for (int i = 0; i < pnum; i++) {
        int min = INT_MAX;
        int max = INT_MIN;

        if (with_files) {
            // Исправлено: увеличен размер буфера
            char filename[32];
            snprintf(filename, sizeof(filename), "min_max_%d.txt", i);
            FILE *file = fopen(filename, "r");
            if (file != NULL) {
                fscanf(file, "%d %d", &min, &max);
                fclose(file);
                remove(filename);
            }
        } else {
            close(pipefd[i][1]);
            read(pipefd[i][0], &min, sizeof(int));
            read(pipefd[i][0], &max, sizeof(int));
            close(pipefd[i][0]);
        }

        if (min < min_max.min) min_max.min = min;
        if (max > min_max.max) min_max.max = max;
    }

    struct timeval finish_time;
    gettimeofday(&finish_time, NULL);

    double elapsed_time = (finish_time.tv_sec - start_time.tv_sec) * 1000.0;
    elapsed_time += (finish_time.tv_usec - start_time.tv_usec) / 1000.0;

    // Освобождаем память
    free(array);
    free(child_pids);
    if (!with_files && pipefd != NULL) {
        for (int i = 0; i < pnum; i++) {
            free(pipefd[i]);
        }
        free(pipefd);
    }

    printf("Min: %d\n", min_max.min);
    printf("Max: %d\n", min_max.max);
    printf("Elapsed time: %fms\n", elapsed_time);
    return 0;
}