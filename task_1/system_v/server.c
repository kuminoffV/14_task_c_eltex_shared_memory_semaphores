#include <stdio.h>
#include <stdlib.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <string.h>
#include <unistd.h>

#define SHM_KEY 1234 // Ключ для сегмента разделяемой памяти
#define SHM_SIZE 256 // Размер сегмента разделяемой памяти

int main() {
    // Создаем сегмент разделяемой памяти
    int shmid = shmget(SHM_KEY, SHM_SIZE, IPC_CREAT | 0666);
    if (shmid < 0) {
        perror("shmget");
        exit(EXIT_FAILURE);
    }

    // Присоединяемся к сегменту разделяемой памяти
    char *shmaddr = (char *)shmat(shmid, NULL, 0);
    if (shmaddr == (char *)-1) {
        perror("shmat");
        exit(EXIT_FAILURE);
    }

    // Записываем сообщение в разделяемую память
    strcpy(shmaddr, "Hi!");

    // Ждем, пока клиент прочитает сообщение и напишет свой ответ
    while (strcmp(shmaddr, "Hello!") != 0) {
        sleep(1); // Периодически проверяем
    }

    // Выводим ответ клиента на экран
    printf("Received from client: %s\n", shmaddr);

    // Отключаемся от разделяемой памяти
    shmdt(shmaddr);

    // Удаляем сегмент разделяемой памяти
    shmctl(shmid, IPC_RMID, NULL);

    return 0;
}

