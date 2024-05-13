#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include <sys/wait.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <termios.h>
#include <signal.h>
#include <curses.h>
#include <dirent.h>
#include <time.h>
#include <malloc.h>
#include <sys/ioctl.h>

#define error_func(a)                      \
    do                                     \
    {                                      \
        if (-1 == a)                       \
        {                                  \
            printf("line:%d\n", __LINE__); \
            perror("error");               \
            exit(EXIT_FAILURE);            \
        }                                  \
    } while (0)

#define PRIOR_SEND_QUERTY_OR_message 5 // приоритет (запрос на сервер)
#define PRIOR_RECEPTION 6              // приоритет (ответ от сервера)
#define MAX_USERS 10                   // максимальное количетсво пользователей
#define MAX_message 40                 // максимальное количетсво сообщей

#define KEY_NAME_TO_SERVER "key_to_server"
#define KEY_NAME_FROM_SERVER "key_from_server"

// структура сообщейний пользователей и рассылки
struct send_and_request
{
    long mtype;
    char message[50];
    char users[10][20];
    int amount_users; // количетсво пользователей в сети
    int num_user;     //- номер пользователя на сервере
    int type_message; //- тип сообщения
                      //-1 - ответ: сервер переполнен
                      // 0 - запрос: вход
                      // 1 - запрос: выход
                      // 2 - ошибка соединения
                      // 3 - отправить новое сообщение
                      // 4 - ответ: новый пользователь
                      // 5 - ответ: новое сообщение
                      // 6 - отключить вервер
    int priority;     // приоритет сообщений для пользователя
                      // случай переполнения чата (-1)
};

int main(void)
{

    key_t key_message_from_server, key_message_to_server;

    // идентификаторы разделяемой памяти и семафоров
    int id_shm_get, id_shm_send, id_sem;

    // принятие сообщений
    struct send_and_request *point_shm_send;

    // информация о сеансах
    struct send_and_request *point_shm_get;

    // проверка заверешния дочернего процесса
    int status;
    ssize_t status_rcv;

    // идентификатор последнего пользователя
    int last_user = 0;

    // счетчики
    int i = 0, i2 = 0;

    // идентификатор работы сервера
    int work = 1;

    // индентификатор пользователя на сервере
    int num_user_server;

    // блокировка сервера, пока не придут новое сообщение
    struct sembuf lock_message = {1, 0, 0};
    struct sembuf lock_get_message[3] = {{3, 0, 0}, {2, 1, 0}, {4, -1, 0}};

    // ключи для разделяемой памяти и для семафоров
    key_message_from_server = ftok(KEY_NAME_FROM_SERVER, 6);
    error_func(key_message_from_server);
    key_message_to_server = ftok(KEY_NAME_TO_SERVER, 6);
    error_func(key_message_to_server);

    // 1) семафор вохможности отправки сообщения на сервер
    // 2) семафор (ожидание сообщений)
    // 3) уведомляем пользователей, о новом сообщении
    id_sem = semget(key_message_from_server, 10, IPC_CREAT | 0666);
    error_func(id_sem);

    // идентификатор на область памяти
    id_shm_get = shmget(key_message_to_server, sizeof(struct send_and_request), IPC_CREAT | 0666);
    error_func(id_shm_get);
    id_shm_send = shmget(key_message_from_server, sizeof(struct send_and_request), IPC_CREAT | 0666);
    error_func(id_shm_send);

    // указатель на разделяемую память и защищаем эту область
    point_shm_send = (struct send_and_request *)shmat(id_shm_send, NULL, 0);
    point_shm_get = (struct send_and_request *)shmat(id_shm_get, NULL, 0);

    memset(point_shm_get, 0, sizeof(struct send_and_request));
    memset(point_shm_send, 0, sizeof(struct send_and_request));

    for (i = 0; i < 10; i++)
    {
        status = semctl(id_sem, i, SETVAL, 15);
        error_func(status);
    }

    printf("Server has started and is starting to work\n");

    // заявки от пользователей
    while (1 == work)
    {
        sleep(1);
        status = semctl(id_sem, 1, SETVAL, 1);
        error_func(status);

        // разрешаем пользователю прислать новое сообщение
        status = semctl(id_sem, 0, SETVAL, 0);
        error_func(status);

        // ждем получения сообщений
        status = semop(id_sem, &lock_message, 1);
        error_func(status);

        switch (point_shm_get->type_message)
        {
        // запрос на вход
        case 0:
            printf("New user %s is trying to log in\n", point_shm_get->users[0]);
            if (10 >= point_shm_send->amount_users)
            {

                // увеличиваем счетчик максимального количетсво пользователей
                point_shm_send->amount_users++;

                // добавляем нового пользователя
                strcpy(point_shm_send->users[last_user], point_shm_get->users[0]);
                last_user++;

                // отправляем по приоритету нового пользователя в системе
                point_shm_send->type_message = 4;
                // сообщение, о новом пользователе
                // ждем, пока пользователи прочитают сообщения
            }
            else
            {
                // переполнение сервера
                point_shm_send->type_message = -4;
            }

            status = semctl(id_sem, 3, SETVAL, last_user);
            error_func(status);
            status = semctl(id_sem, 4, SETVAL, 1);
            error_func(status);

            // разрешаем пользователям читать сообщения
            status = semctl(id_sem, 2, SETVAL, 0);
            error_func(status);

            // ожидание пока пользователи прочитают сообщения
            status = semop(id_sem, lock_get_message, 3);
            error_func(status);

            break;

        // запрос на выход
        case 1:
            printf("User %s is trying to log out of the session\n", point_shm_get->users[0]);

            // ищем пользователя
            for (i = 0; i < 10; i++)
            {
                if (!strcmp(point_shm_get->users[0], point_shm_send->users[i]))
                {
                    i2 = i;
                }
            }

            // перемещение пользователей на место ушедшего
            strcpy(point_shm_send->users[i2], point_shm_send->users[last_user - 1]);

            // зануляем последнего пользователя
            memset(point_shm_send->users[last_user - 1], 0, sizeof(point_shm_send->users[last_user - 1]));
            point_shm_send->amount_users--;

            // отправляем список пользователей
            point_shm_send->type_message = -4;

            status = semctl(id_sem, 3, SETVAL, last_user);
            error_func(status);
            status = semctl(id_sem, 4, SETVAL, 1);
            error_func(status);

            // разрешаем пользователям читать сообщения;
            status = semctl(id_sem, 2, SETVAL, 0);
            error_func(status);

            // ожидание пока пользователи прочитают сообщения
            status = semop(id_sem, lock_get_message, 3);
            error_func(status);

            break;
        case 3:

            memset(point_shm_send->message, 0, sizeof(point_shm_send->message));
            strcat(point_shm_send->message, point_shm_get->users[0]);
            strcat(point_shm_send->message, ": ");
            strcat(point_shm_send->message, point_shm_get->message);

            point_shm_send->type_message = 5;

            status = semctl(id_sem, 3, SETVAL, last_user);
            error_func(status);
            status = semctl(id_sem, 4, SETVAL, 1);
            error_func(status);

            // разрешаем пользователям читать сообщения
            status = semctl(id_sem, 2, SETVAL, 0);
            error_func(status);

            // ожидание пока пользователи прочитают сообщения
            status = semop(id_sem, lock_get_message, 3);
            error_func(status);

            break;
        //- выключение сервера
        case 6:
            printf("User %s is trying to shut down the server\n", point_shm_get->users[0]);
            point_shm_send->type_message = -1;

            status = semctl(id_sem, 3, SETVAL, last_user);
            error_func(status);
            status = semctl(id_sem, 4, SETVAL, 1);
            error_func(status);

            // разрешаем пользователям читать сообщения
            status = semctl(id_sem, 2, SETVAL, 0);
            error_func(status);

            // ожидание пока пользователи прочитают сообщения
            status = semop(id_sem, lock_get_message, 3);
            error_func(status);

            work = 0;

            break;

        default:

            break;
        }
    }
    exit(EXIT_SUCCESS);
}
