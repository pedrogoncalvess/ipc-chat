#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <unistd.h>
#include <signal.h>
#include <time.h>

#define MSG_SIZE 1024
#define COLOR_RESET "\033[0m"
#define COLOR_USER2 "\033[1;36m"
#define COLOR_USER1 "\033[1;35m"

struct msgbuf {
    long mtype;
    char mtext[MSG_SIZE];
};

int msgid;

void get_timestamp(char* buffer, size_t size) {
    time_t now = time(NULL);
    struct tm *t = localtime(&now);
    strftime(buffer, size, "%H:%M:%S", t);
}

void cleanup_and_exit() {
    printf("\n[User2 encerrado]\n");
    exit(0);
}

void handle_sigint(int sig) {
    cleanup_and_exit();
}

void print_help() {
    printf("\nComandos disponíveis:\n");
    printf("  /clear   - Limpa o terminal\n");
    printf("  /help    - Mostra esta ajuda\n");
    printf("  /exit    - Sai do chat\n\n");
}

int main() {
    signal(SIGINT, handle_sigint);

    key_t key = ftok("msgqueue", 65);
    msgid = msgget(key, 0666 | IPC_CREAT);
    if (msgid == -1) {
        perror("msgget");
        exit(EXIT_FAILURE);
    }

    printf("🟢 User2 ligado ao chat.\n\n");

    struct msgbuf msg;
    char input[MSG_SIZE];
    char timestamp[32];

    pid_t pid = fork();

    if (pid == 0) {
        // Filho: enviar para User1 (mtype = 1)
        while (1) {
            printf(COLOR_USER2 "User2 > " COLOR_RESET);
            fflush(stdout);

            if (fgets(input, MSG_SIZE, stdin) == NULL) {
                cleanup_and_exit();
            }
            input[strcspn(input, "\n")] = 0;

            if (strlen(input) == 0) continue;

            if (strcmp(input, "/exit") == 0) {
                msg.mtype = 1;
                strcpy(msg.mtext, "/exit");
                msgsnd(msgid, &msg, strlen(msg.mtext) + 1, 0);
                kill(getppid(), SIGINT);
                exit(0);
            }

            if (strcmp(input, "/clear") == 0) {
                system("clear");
                continue;
            }

            if (strcmp(input, "/help") == 0) {
                print_help();
                continue;
            }

            msg.mtype = 1;
            strncpy(msg.mtext, input, MSG_SIZE);
            msg.mtext[MSG_SIZE - 1] = '\0';

            if (msgsnd(msgid, &msg, strlen(msg.mtext) + 1, 0) == -1) {
                perror("msgsnd");
                cleanup_and_exit();
            }
        }
    } else {
        // Pai: receber de User1 (mtype = 2)
        while (1) {
            ssize_t ret = msgrcv(msgid, &msg, sizeof(msg.mtext), 2, 0);
            if (ret <= 0) {
                printf("\n[Falha ao receber mensagem ou fila removida. A sair...]\n");
                cleanup_and_exit();
            }

            if (strcmp(msg.mtext, "/exit") == 0) {
                printf("\n[User1 saiu do chat]\n");
                cleanup_and_exit();
            }

            get_timestamp(timestamp, sizeof(timestamp));
            printf("\r" COLOR_USER1 "[User1 | %s] %s\n" COLOR_RESET, timestamp, msg.mtext);
            printf(COLOR_USER2 "User2 > " COLOR_RESET);
            fflush(stdout);
        }
    }

    return 0;
}