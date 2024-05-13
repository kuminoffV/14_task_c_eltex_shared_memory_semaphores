# Компилятор
CC = gcc

# Флаги компилятора
CFLAGS = -Wall -Wextra -std=c99

# Цели по умолчанию
all: server client

# Компиляция программы сервера
server: server.c
	$(CC) $(CFLAGS) -o server server.c

# Компиляция программы клиента
client: client.c
	$(CC) $(CFLAGS) -o client client.c

# Цель для очистки скомпилированных файлов
clean:
	rm -f server client

