#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include <time.h>
#include <pthread.h>
#include <limits.h>  // INT_MAX�� INT_MIN ����� ���� �߰�

#define PORT 8080
#define MAX_CLIENTS 3
#define SHM_KEY 5678
#define SEM_KEY 1234

// Ŭ���̾�Ʈ�� ���� ���� ������ ����ü
struct client_data {
    int left_num;
    int right_num;
    char op;
    char student_info[50]; // OSNW2024, �й�, �̸� ���� ���� ���ڿ�
};

struct result_data {
    int result;
    int min;
    int max;
    struct tm timestamp;
    struct sockaddr_in server_ip;
};

// Ŭ���̾�Ʈ���� ������ �л� ������ ��� ��û�� ó���ϴ� �Լ�
void* handle_client(void* arg) {
    int client_sock = *(int*)arg;
    free(arg);

    struct client_data data;
    struct result_data result;
    int shm_id = shmget(SHM_KEY, sizeof(struct result_data), IPC_CREAT | 0666);
    struct result_data* shm_result = shmat(shm_id, NULL, 0);

    // �ʱ� min, max ����
    result.min = INT_MAX;
    result.max = INT_MIN;

    while (1) {
        // Ŭ���̾�Ʈ�κ��� ������ ����
        if (recv(client_sock, &data, sizeof(data), 0) <= 0) {
            perror("recv failed");
            break;
        }

        // ���� ����
        if (data.left_num == 0 && data.right_num == 0 && data.op == '$') {
            break;
        }

        // ��� ����
        if (data.op == '+') {
            result.result = data.left_num + data.right_num;
        }
        else if (data.op == '-') {
            result.result = data.left_num - data.right_num;
        }
        else if (data.op == 'x') {
            result.result = data.left_num * data.right_num;
        }
        else if (data.op == '/') {
            result.result = (data.right_num != 0) ? (data.left_num / data.right_num) : 0;
        }

        // min, max ������Ʈ
        if (result.result < result.min) {
            result.min = result.result;
        }
        if (result.result > result.max) {
            result.max = result.result;
        }

        // ���� �ð� ��������
        time_t rawtime = time(NULL);
        result.timestamp = *localtime(&rawtime);

        // ������ IP �ּ� ����
        socklen_t addr_len = sizeof(result.server_ip);
        getsockname(client_sock, (struct sockaddr*)&result.server_ip, &addr_len);

        // ���� �޸𸮿� ��� ����
        memcpy(shm_result, &result, sizeof(result));

        // Ŭ���̾�Ʈ���� ��� ����
        send(client_sock, shm_result, sizeof(result), 0);

        // 10�� ���
        sleep(10);
    }

    close(client_sock);
    shmdt(shm_result);
    return NULL;
}

int main() {
    int server_sock, client_sock;
    struct sockaddr_in server_addr, client_addr;
    pthread_t tid;

    // ���� ����
    server_sock = socket(AF_INET, SOCK_STREAM, 0);
    if (server_sock < 0) {
        perror("socket creation failed");
        exit(EXIT_FAILURE);
    }

    // ���� �ּ� ����
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(PORT);

    // ���ε�
    if (bind(server_sock, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        perror("bind failed");
        close(server_sock);
        exit(EXIT_FAILURE);
    }

    // ������
    listen(server_sock, MAX_CLIENTS);
    printf("Server is listening on port %d\n", PORT);

    while (1) {
        socklen_t client_len = sizeof(client_addr);
        // Ŭ���̾�Ʈ ���� ����
        client_sock = accept(server_sock, (struct sockaddr*)&client_addr, &client_len);
        if (client_sock < 0) {
            perror("accept failed");
            continue;
        }

        // Ŭ���̾�Ʈ�� ó���ϴ� ������ ����
        int* pclient = malloc(sizeof(int));
        *pclient = client_sock;
        pthread_create(&tid, NULL, handle_client, pclient);
    }

    close(server_sock);
    return 0;
}
