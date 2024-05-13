# Имя компилятора
CC = gcc

# Флаги компилятора
CFLAGS = -Wall -Wextra -O2

# Названия целевых объектов (targets)
SERVER_TARGET = server
CLIENT_TARGET = client

# Файлы исходного кода
SERVER_SRC = server.c
CLIENT_SRC = client.c

# Создание исполняемого файла для сервера
$(SERVER_TARGET): $(SERVER_SRC)
	$(CC) $(CFLAGS) -o $(SERVER_TARGET) $(SERVER_SRC)

# Создание исполняемого файла для клиента
$(CLIENT_TARGET): $(CLIENT_SRC)
	$(CC) $(CFLAGS) -o $(CLIENT_TARGET) $(CLIENT_SRC)

# Очистка: удаление исполняемых файлов
clean:
	rm -f $(SERVER_TARGET) $(CLIENT_TARGET)

# Очистка всех временных файлов
distclean: clean
	rm -f *.o

