CC=gcc
CFLAGS=-I. -Wall -Wextra -pthread
TARGETS=parallel_min_max zombie_demo parallel_sum

# Target по умолчанию - собирает все программы
all: $(TARGETS)

# Сборка parallel_min_max
parallel_min_max: parallel_min_max.o utils.o find_min_max.o
	$(CC) -o $@ $^ $(CFLAGS)

# Сборка zombie_demo
zombie_demo: zombie_demo.o
	$(CC) -o $@ $^ $(CFLAGS)

# Сборка parallel_sum
parallel_sum: parallel_sum.o utils.o sum.o
	$(CC) -o $@ $^ $(CFLAGS)

# Компиляция объектных файлов
parallel_min_max.o: parallel_min_max.c utils.h find_min_max.h
	$(CC) -c parallel_min_max.c $(CFLAGS)

utils.o: utils.c utils.h
	$(CC) -c utils.c $(CFLAGS)

find_min_max.o: find_min_max.c find_min_max.h utils.h
	$(CC) -c find_min_max.c $(CFLAGS)

zombie_demo.o: zombie_demo.c
	$(CC) -c zombie_demo.c $(CFLAGS)

parallel_sum.o: parallel_sum.c utils.h sum.h
	$(CC) -c parallel_sum.c $(CFLAGS)

sum.o: sum.c sum.h
	$(CC) -c sum.c $(CFLAGS)

# Очистка скомпилированных файлов
clean:
	rm -f *.o $(TARGETS) min_max_*.txt

# Пересборка
rebuild: clean all

.PHONY: all clean rebuild