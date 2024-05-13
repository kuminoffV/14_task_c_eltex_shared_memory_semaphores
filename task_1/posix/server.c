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

    // Создание общего сегмента памяти
    shm_fd = shm_open(SHM_NAME, O_CREAT | O_RDWR, 0666);
    if (shm_fd == -1)
        error("shm_open");

    // Установка размера общего сегмента памяти
    if (ftruncate(shm_fd, SHM_SIZE) == -1)
        error("ftruncate");

    // Отображение общего сегмента памяти в адресное пространство процесса
    shm_ptr = mmap(0, SHM_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
    if (shm_ptr == MAP_FAILED)
        error("mmap");

    // Создание семафора
    sem = sem_open(SEM_NAME, O_CREAT, 0666, 0);
    if (sem == SEM_FAILED)
        error("sem_open");

    // Запись сообщения в общий сегмент памяти
    strcpy(shm_ptr, "Hi!");

    printf("Сервер: Ожидание ответа от клиента...\n");
    // Ожидание ответа от клиента
    sem_wait(sem);

    // Чтение сообщения клиента из общего сегмента памяти
    printf("Сервер: Получено сообщение от клиента: %s\n", shm_ptr);

    // Очистка
    munmap(shm_ptr, SHM_SIZE);
    shm_unlink(SHM_NAME);
    sem_close(sem);
    sem_unlink(SEM_NAME);

    return 0;
}
