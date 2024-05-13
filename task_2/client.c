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

//- функция, что делает дефолтные надписи в окнах;
void start_logo(WINDOW *wnd_see_message, WINDOW *wnd_urers, WINDOW *wnd_send_message);

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
#define MAX_message 40                 // максимальное количетсво сообщейний

#define KEY_NAME_TO_SERVER "key_to_server"
#define KEY_NAME_FROM_SERVER "key_from_server"

// структура сообщейний пользователей и рассылки
struct send_and_request
{
    long mtype;
    char message[50];
    char users[10][20];
    int amount_users; // количетсво пользователей
    int num_user;     //- номер пользователя
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

// прототип функции оформления окон
void start_logo(WINDOW *wnd_see_message, WINDOW *wnd_urers, WINDOW *wnd_send_message);

void sig_winch(int signo)
{
    struct winsize size;
    ioctl(fileno(stdout), TIOCGWINSZ, (char *)&size);
    resizeterm(size.ws_row, size.ws_col);
}

int main(void)
{
    // указатели на окна
    WINDOW *wnd_send_message;
    WINDOW *wnd_urers;
    WINDOW *wnd_see_message;

    // координаты
    int x_names = 0, y_names = 0;
    int x_message = 0, y_message = 0;

    // буфер имен
    char buf_name[20];
    char buf_message[28];

    // счетчик циклов
    int i;

    // указатель на область разделенной памяти
    struct send_and_request *point_shm_send;
    struct send_and_request *point_shm_get;
    key_t key_message_from_server, key_message_to_server;

    // идентификаторы разделяемой памяти и семафоров
    int id_shm_get, id_shm_send, id_sem;

    // для создания нового процесса
    // для разделения поля имен и сообщений
    pid_t pid;

    // проверка завершения дочернего процесса
    int status;

    // проверка наличия пользователя на сервере
    int yes_or_no;

    // ожидание возможность отправки сообщений
    struct sembuf lock_1[2] = {{0, 0, 0}, {0, 1, 0}};

    // сообщаем серверу, что послали новое сообщение;
    struct sembuf lock_2 = {1, -1, 0};

    // сообщение от сервера, что пришло новое сообщение
    struct sembuf lock_3 = {2, 0, 0};

    // сообщаем серверу, что прочитали сообщение
    struct sembuf lock_4 = {3, -1, 0};
    struct sembuf lock_5 = {4, 0, 0};

    //- создаем ключи для разделяемой памяти и для семафоров;
    key_message_from_server = ftok(KEY_NAME_FROM_SERVER, 6);
    error_func(key_message_from_server);

    key_message_to_server = ftok(KEY_NAME_TO_SERVER, 6);
    error_func(key_message_from_server);

    // идентификатор на область памяти
    id_shm_get = shmget(key_message_from_server, sizeof(struct send_and_request), IPC_EXCL | 0666);
    error_func(id_shm_get);
    id_shm_send = shmget(key_message_to_server, sizeof(struct send_and_request), IPC_EXCL | 0666);
    error_func(id_shm_send);

    // указатель на разделяемую память
    point_shm_get = (struct send_and_request *)shmat(id_shm_get, NULL, 0);
    point_shm_send = (struct send_and_request *)shmat(id_shm_send, NULL, 0);

    // 1) семафор отправки на сервер
    // 2) семафор принятия от сервера
    // 3) уведомление от сервера, что пришло новое сообщение
    // 4) увеомляем сервер, что прочитали сообщение
    id_sem = semget(key_message_from_server, 10, IPC_EXCL | 0666);
    error_func(id_sem);

    // инициализирем  ncurses и перевод терминала в нужный режим
    initscr();

    // устанавливаем обработчик сигнала SIGWINCH
    signal(SIGWINCH, sig_winch);

    // отключаем отображение курсора
    curs_set(0);

    // отключаем отображение символов на экране
    noecho();

    // обновление экрана
    refresh();

    // создание двух окон (сообщений и списка пользователей)
    wnd_send_message = newwin((stdscr->_maxy / 10) - 1, (stdscr->_maxx / 4) - 3, (stdscr->_maxy / 4) + 22, 3);
    wnd_urers = newwin((stdscr->_maxy) - 10, (stdscr->_maxx / 3.5) - 1, 3, (stdscr->_maxx / 3) - 15 + 20 + 20);
    wnd_see_message = newwin((stdscr->_maxy / 3) + 20, (stdscr->_maxx / 4) - 3 + 25, 3, 3);
    wmove(wnd_see_message, 4, 2);
    wprintw(wnd_see_message, "Input your name");
    start_logo(wnd_see_message, wnd_urers, wnd_send_message);

    // обновление окон
    wrefresh(wnd_see_message);
    wrefresh(wnd_urers);
    wrefresh(wnd_send_message);
    echo();

    curs_set(1);

    // первый элемент массива имен -  имя пользователя, которое отправим серверу
    wmove(wnd_send_message, 1, 1);
    wgetnstr(wnd_send_message, buf_name, 20);

    // очистка полей после отправки сообщений
    werase(wnd_send_message);
    werase(wnd_urers);
    werase(wnd_see_message);

    // обновляем поля
    start_logo(wnd_see_message, wnd_urers, wnd_send_message);

    // запрос серверу на вход
    status = semop(id_sem, lock_1, 2);
    error_func(status);
    point_shm_send->type_message = 0;
    strcpy(point_shm_send->users[0], buf_name);
    status = semop(id_sem, &lock_2, 1);
    error_func(status);

    werase(wnd_send_message);
    start_logo(wnd_see_message, wnd_urers, wnd_send_message);
    wrefresh(wnd_send_message);

    // будет два процесса
    // процесс для посылки сообщения
    pid = fork();
    error_func(pid);

    if (0 == pid)
    {

        while (1)
        {
            sleep(1);

            // обработка строки
            wgetnstr(wnd_send_message, buf_message, 28);

            // обновление строк
            werase(wnd_send_message);
            start_logo(wnd_see_message, wnd_urers, wnd_send_message);
            wrefresh(wnd_send_message);

            status = semop(id_sem, lock_1, 2);
            error_func(status);

            if (0 == strcmp(buf_message, "exit"))
            {
                point_shm_send->type_message = 1;
                strcpy(point_shm_send->users[0], buf_name);
                status = semop(id_sem, &lock_2, 1);
                error_func(status);
                break;
            }
            else if (0 == strcmp(buf_message, "fin"))
            {
                point_shm_send->type_message = 6;
                strcpy(point_shm_send->users[0], buf_name);
                status = semop(id_sem, &lock_2, 1);
                error_func(status);
                break;
            }
            else
            {
                point_shm_send->type_message = 3;
                strcpy(point_shm_send->users[0], buf_name);
                strcpy(point_shm_send->message, buf_message);
                status = semop(id_sem, &lock_2, 1);
                error_func(status);
            }

            werase(wnd_send_message);
            start_logo(wnd_see_message, wnd_urers, wnd_send_message);
            wrefresh(wnd_send_message);
        }
    }
    else if (0 < pid)
    {

        // начальные координаты
        x_message = 1;
        y_message = 4;

        // убираем лишнюю информацию в окнах
        werase(wnd_urers);
        werase(wnd_see_message);
        start_logo(wnd_see_message, wnd_urers, wnd_send_message);
        wrefresh(wnd_see_message);

        // цикл для передачи данных от сервера
        while (1)
        {
            sleep(1);

            status = semop(id_sem, &lock_3, 1);
            error_func(status);

            if (-4 == point_shm_get->type_message)
            {

                // ищем пользователя на сервере
                for (i = 0; i < 10; i++)
                {
                    if (!strcmp(buf_name, point_shm_get->users[i]))
                    {
                        yes_or_no = 1;
                    }
                }
                if (1 == yes_or_no)
                {
                    continue;
                }
                else
                {
                    printf("The server is full or closed\n");
                    break;
                }
            }
            else if (4 == point_shm_get->type_message)
            {
                // обработка окна пользователей
                werase(wnd_urers);
                start_logo(wnd_see_message, wnd_urers, wnd_send_message);
                for (i = 0; i < point_shm_get->amount_users; i++)
                {
                    wmove(wnd_urers, 4 + i, 1);
                    wprintw(wnd_urers, "%s", point_shm_get->users[i]);
                    wrefresh(wnd_urers);
                }
            }
            else if (5 == point_shm_get->type_message)
            {

                // обработка окна сообщений
                if (MAX_message + 4 == y_message)
                {
                    werase(wnd_see_message);
                    start_logo(wnd_see_message, wnd_urers, wnd_send_message);
                    x_message = 1;
                    y_message = 4;
                }

                wmove(wnd_see_message, y_message, x_message + 1);
                wprintw(wnd_see_message, "%s", point_shm_get->message);
                wrefresh(wnd_see_message);

                y_message++;
            }
            else if (-1 == point_shm_get->type_message)
            {

                // узнали о завершении
                status = semop(id_sem, &lock_4, 1);
                error_func(status);
                status = semop(id_sem, &lock_5, 1);
                error_func(status);

                break;
            }

            wmove(wnd_send_message, 1, 1);
            wrefresh(wnd_send_message);

            // прочитали и едем дальше
            status = semop(id_sem, &lock_4, 1);
            error_func(status);
            status = semop(id_sem, &lock_5, 1);
            error_func(status);
        }

        pid = wait(&status);
        error_func(pid);
        if (!WIFEXITED(status))
        {
            printf("Child process (1) terminated incorrectly");
        }

        delwin(wnd_send_message);
        delwin(wnd_urers);
        delwin(wnd_see_message);

        endwin();
    }

    exit(EXIT_SUCCESS);
}

// функция оформления окон
void start_logo(WINDOW *wnd_see_message, WINDOW *wnd_urers, WINDOW *wnd_send_message)
{
    // обводка окон
    box(wnd_send_message, '|', '-');
    box(wnd_urers, '|', '-');
    box(wnd_see_message, '|', '-');

    // оформление окон
    wmove(wnd_see_message, 1, 2);
    wprintw(wnd_see_message, "Chat");
    wmove(wnd_see_message, 2, 2);
    wprintw(wnd_see_message, "--------------------------------------");
    wmove(wnd_urers, 1, 2);
    wprintw(wnd_urers, "Users");
    wmove(wnd_urers, 2, 2);
    wprintw(wnd_urers, "----------------");
}
