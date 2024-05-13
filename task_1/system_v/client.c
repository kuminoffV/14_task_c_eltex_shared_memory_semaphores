#include <stdio.h>
#include <stdlib.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <string.h>

#define SHM_KEY 1234 // Ключ для сегмента разделяемой памяти
#define SHM_SIZE 256 // Размер сегмента разделяемой памяти

int main() {
    // Подключаемся к существующему сегменту разделяемой памяти
    int shmid = shmget(SHM_KEY, SHM_SIZE, 0666);
    if (shmid < 0) {
        perror("shmget");
        exit(EXIT_FAILURE);
    }

    // Присоединяемся к разделяемой памяти
    char *shmaddr = (char *)shmat(shmid, NULL, 0);
    if (shmaddr == (char *)-1) {
        perror("shmat");
        exit(EXIT_FAILURE);
    }

    // Читаем сообщение от сервера
    printf("Received from server: %s\n", shmaddr);

    // Пишем ответ в разделяемую память
    strcpy(shmaddr, "Hello!");

    // Отключаемся от разделяемой памяти
    shmdt(shmaddr);

    return 0;
}

