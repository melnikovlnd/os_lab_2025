#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

int main(int argc, char *argv[]) {
    if (argc != 3) {
        printf("Usage: %s <seed> <array_size>\n", argv[0]);
        return 1;
    }

    pid_t pid = fork();
    
    if (pid == -1) {
        perror("fork failed");
        return 1;
    } else if (pid == 0) {
        // Дочерний процесс
        printf("Child process: Starting sequential_min_max...\n");
        
        // Запускаем sequential_min_max с правильными аргументами
        // Без -- перед параметрами
        execl("./sequential_min_max", "sequential_min_max", argv[1], argv[2], NULL);
        
        perror("execl failed");
        return 1;
    } else {
        // Родительский процесс
        printf("Parent process: Waiting for child (PID: %d) to complete...\n", pid);
        
        int status;
        waitpid(pid, &status, 0);
        
        if (WIFEXITED(status)) {
            printf("Parent process: Child completed with exit code %d\n", WEXITSTATUS(status));
        } else {
            printf("Parent process: Child terminated abnormally\n");
        }
    }
    
    return 0;
}