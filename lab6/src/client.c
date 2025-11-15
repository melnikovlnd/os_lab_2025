#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <errno.h>
#include <getopt.h>
#include <netdb.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <pthread.h>
#include <arpa/inet.h>

#include "common.h"

bool ConvertStringToUI64(const char *str, int *val) {
    char *end = NULL;
    unsigned long long i = strtoull(str, &end, 10);
    if (errno == ERANGE) {
        return false;
    }
    if (errno != 0)
        return false;
    *val = i;
    return true;
}


struct ClientArgs {
    struct Server server;
    uint64_t begin;
    uint64_t end;
    uint64_t mod;
    uint64_t result;
};

void* ProcessServer(void* args) {
    struct ClientArgs* cargs = (struct ClientArgs*)args;
    
    struct sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(cargs->server.port);
    
    // Исправляем получение IP адреса
    if (inet_pton(AF_INET, cargs->server.ip, &server_addr.sin_addr) <= 0) {
        fprintf(stderr, "Invalid address: %s\n", cargs->server.ip);
        cargs->result = 1; // Нейтральный элемент для умножения
        return NULL;
    }

    int sck = socket(AF_INET, SOCK_STREAM, 0);
    if (sck < 0) {
        fprintf(stderr, "Socket creation failed!\n");
        cargs->result = 1;
        return NULL;
    }

    if (connect(sck, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        fprintf(stderr, "Connection to %s:%d failed\n", cargs->server.ip, cargs->server.port);
        close(sck);
        cargs->result = 1;
        return NULL;
    }

    char task[sizeof(uint64_t) * 3];
    memcpy(task, &cargs->begin, sizeof(uint64_t));
    memcpy(task + sizeof(uint64_t), &cargs->end, sizeof(uint64_t));
    memcpy(task + 2 * sizeof(uint64_t), &cargs->mod, sizeof(uint64_t));

    if (send(sck, task, sizeof(task), 0) < 0) {
        fprintf(stderr, "Send failed to %s:%d\n", cargs->server.ip, cargs->server.port);
        close(sck);
        cargs->result = 1;
        return NULL;
    }

    char response[sizeof(uint64_t)];
    if (recv(sck, response, sizeof(response), 0) < 0) {
        fprintf(stderr, "Receive failed from %s:%d\n", cargs->server.ip, cargs->server.port);
        close(sck);
        cargs->result = 1;
        return NULL;
    }

    memcpy(&cargs->result, response, sizeof(uint64_t));
    printf("Received from %s:%d: %lu\n", cargs->server.ip, cargs->server.port, cargs->result);

    close(sck);
    return NULL;
}

int main(int argc, char **argv) {
    uint64_t k = 0;
    uint64_t mod = 0;
    char servers_file[255] = {'\0'};
    bool k_set = false, mod_set = false, servers_set = false;

    while (true) {
        static struct option options[] = {{"k", required_argument, 0, 0},
                                          {"mod", required_argument, 0, 0},
                                          {"servers", required_argument, 0, 0},
                                          {0, 0, 0, 0}};

        int option_index = 0;
        int c = getopt_long(argc, argv, "", options, &option_index);

        if (c == -1)
            break;

        switch (c) {
        case 0: {
            switch (option_index) {
            case 0:
                if (!ConvertStringToUI64(optarg, &k)) {
                    fprintf(stderr, "Invalid k value: %s\n", optarg);
                    return 1;
                }
                k_set = true;
                break;
            case 1:
                if (!ConvertStringToUI64(optarg, &mod)) {
                    fprintf(stderr, "Invalid mod value: %s\n", optarg);
                    return 1;
                }
                mod_set = true;
                break;
            case 2:
                strncpy(servers_file, optarg, sizeof(servers_file) - 1);
                servers_file[sizeof(servers_file) - 1] = '\0';
                servers_set = true;
                break;
            default:
                printf("Index %d is out of options\n", option_index);
            }
        } break;

        case '?':
            printf("Arguments error\n");
            break;
        default:
            fprintf(stderr, "getopt returned character code 0%o?\n", c);
        }
    }

    if (!k_set || !mod_set || !servers_set) {
        fprintf(stderr, "Using: %s --k 1000 --mod 5 --servers /path/to/file\n",
                argv[0]);
        return 1;
    }

    if (k == 0 || mod == 0) {
        fprintf(stderr, "k and mod must be positive numbers\n");
        return 1;
    }

    // Read servers from file
    FILE* file = fopen(servers_file, "r");
    if (file == NULL) {
        fprintf(stderr, "Cannot open servers file: %s\n", servers_file);
        return 1;
    }

    struct Server servers[100];
    int servers_num = 0;
    char line[255];
    
    while (fgets(line, sizeof(line), file) != NULL && servers_num < 100) {
        // Убираем символ новой строки
        line[strcspn(line, "\n")] = 0;
        
        char* colon = strchr(line, ':');
        if (colon != NULL) {
            *colon = '\0';
            strncpy(servers[servers_num].ip, line, sizeof(servers[servers_num].ip) - 1);
            servers[servers_num].port = atoi(colon + 1);
            if (servers[servers_num].port > 0) {
                servers_num++;
            }
        }
    }
    fclose(file);

    if (servers_num == 0) {
        fprintf(stderr, "No valid servers found in file\n");
        return 1;
    }

    printf("Found %d servers\n", servers_num);
    printf("Computing %lu! mod %lu\n", k, mod);

    // Distribute work among servers
    pthread_t threads[servers_num];
    struct ClientArgs args[servers_num];
    
    uint64_t segment = k / servers_num;
    
    for (int i = 0; i < servers_num; i++) {
        args[i].server = servers[i];
        args[i].begin = i * segment + 1;
        args[i].end = (i == servers_num - 1) ? k : (i + 1) * segment;
        args[i].mod = mod;
        args[i].result = 1;

        if (pthread_create(&threads[i], NULL, ProcessServer, (void*)&args[i])) {
            fprintf(stderr, "Error creating thread for server %s:%d\n", 
                    servers[i].ip, servers[i].port);
            args[i].result = 1;
        }
    }

    // Wait for all threads to complete
    uint64_t total = 1;
    for (int i = 0; i < servers_num; i++) {
        pthread_join(threads[i], NULL);
        total = MultModulo(total, args[i].result, mod);
    }

    printf("Final result: %lu! mod %lu = %lu\n", k, mod, total);

    return 0;
}