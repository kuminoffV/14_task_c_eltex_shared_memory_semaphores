#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <semaphore.h>

#define SHM_NAME "/my_shared_memory"
#define SEM_NAME "/my_semaphore"
#define SHM_SIZE 1024

void error(char *msg)
{
    perror(msg);
    exit(EXIT_FAILURE);
}

int main()
{
    int shm_fd;
    char *shm_ptr;
    sem_t *sem;

    // Открытие общего сегмента памяти
    shm_fd = shm_open(SHM_NAME, O_RDWR, 0666);
    if (shm_fd == -1)
        error("shm_open");

    // Отображение общего сегмента памяти в адресное пространство процесса
    shm_ptr = mmap(0, SHM_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
    if (shm_ptr == MAP_FAILED)
        error("mmap");

    // Открытие семафора
    sem = sem_open(SEM_NAME, 0);
    if (sem == SEM_FAILED)
        error("sem_open");

    // Чтение сообщения от сервера
    printf("Клиент: Получено сообщение от сервера: %s\n", shm_ptr);

    // Запись ответа в общий сегмент памяти
    strcpy(shm_ptr, "Hello!");

    // Сигнализирование серверу о готовности ответа
    sem_post(sem);

    // Очистка
    munmap(shm_ptr, SHM_SIZE);
    sem_close(sem);

    return 0;
}
