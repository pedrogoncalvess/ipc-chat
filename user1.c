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
#define COLOR_USER1 "\033[1;35m"
#define COLOR_USER2 "\033[1;36m"

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

void cleanup_and_exit_remove() {
    printf("\n[User1 encerrado]\n");
    msgctl(msgid, IPC_RMID, NULL);
    exit(0);
}

void cleanup_and_exit_no_remove() {
    printf("\n[User1 encerrado]\n");
    exit(0);
}

void handle_sigint(int sig) {
    cleanup_and_exit_remove();
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

    printf("🟢 User1 ligado ao chat.\n\n");

    struct msgbuf msg;
    char input[MSG_SIZE];
    char timestamp[32];

    pid_t pid = fork();

    if (pid == 0) {
        // Filho: enviar mensagens para User2 (mtype = 2)
        while (1) {
            printf(COLOR_USER1 "User1 > " COLOR_RESET);
            fflush(stdout);

            if (fgets(input, MSG_SIZE, stdin) == NULL) {
                cleanup_and_exit_remove();
            }
            input[strcspn(input, "\n")] = 0;

            if (strlen(input) == 0) continue;

            if (strcmp(input, "/exit") == 0) {
                msg.mtype = 2;
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

            msg.mtype = 2;
            strncpy(msg.mtext, input, MSG_SIZE);
            msg.mtext[MSG_SIZE - 1] = '\0';

            if (msgsnd(msgid, &msg, strlen(msg.mtext) + 1, 0) == -1) {
                perror("msgsnd");
                cleanup_and_exit_remove();
            }
        }
    } else {
        // Pai: receber mensagens (mtype = 1)
        while (1) {
            ssize_t ret = msgrcv(msgid, &msg, sizeof(msg.mtext), 1, 0);
            if (ret <= 0) {
                printf("\n[Falha ao receber mensagem ou fila removida. A sair...]\n");
                cleanup_and_exit_no_remove();
            }

            if (strcmp(msg.mtext, "/exit") == 0) {
                printf("\n[User2 saiu do chat]\n");
                cleanup_and_exit_no_remove();
            }

            get_timestamp(timestamp, sizeof(timestamp));
            printf("\r" COLOR_USER2 "[User2 | %s] %s\n" COLOR_RESET, timestamp, msg.mtext);
            printf(COLOR_USER1 "User1 > " COLOR_RESET);
            fflush(stdout);
        }
    }

    return 0;
}