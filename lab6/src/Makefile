# Компилятор и флаги
CC = gcc
CFLAGS = -Wall -Wextra -pthread

# Основные цели
all: client server

# Клиент
client: client.c common.c common.h
	$(CC) $(CFLAGS) -o client client.c common.c

# Сервер
server: server.c common.c common.h
	$(CC) $(CFLAGS) -o server server.c common.c

# Очистка
clean:
	rm -f client server

# Создание примера файла серверов
servers.txt:
	echo "127.0.0.1:20001" > servers.txt
	echo "127.0.0.1:20002" >> servers.txt
	echo "127.0.0.1:20003" >> servers.txt

# Тест
test: all servers.txt
	@echo "Запустите серверы: ./server --port 20001 --tnum 2"
	@echo "Затем клиент: ./client --k 20 --mod 1000 --servers servers.txt"

.PHONY: all clean test