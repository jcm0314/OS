#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <time.h>
#include <pthread.h>
#include <limits.h>

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

// ���� �޸� ����ü
struct shared_memory {
    struct client_data client;
    struct result_data result;
    int ready; // ������ �غ� ���� �÷���
};

// ���ε༭ �Լ�
void* producer(void* arg) {
    int client_sock = *(int*)arg;
    free(arg);

    int shm_id = shmget(SHM_KEY, sizeof(struct shared_memory), IPC_CREAT | 0666);
    struct shared_memory* shm = shmat(shm_id, NULL, 0);

    while (1) {
        // Ŭ���̾�Ʈ�κ��� ������ ����
        if (recv(client_sock, &shm->client, sizeof(shm->client), 0) <= 0) {
            perror("recv failed");
            break;
        }

        // ���� ����
        if (shm->client.left_num == 0 && shm->client.right_num == 0 && shm->client.op == '$') {
            break;
        }

        // ������ �غ� ���� ����
        shm->ready = 1;
    }

    close(client_sock);
    shmdt(shm);
    return NULL;
}

// ������ �Լ�
void* consumer(void* arg) {
    int shm_id = shmget(SHM_KEY, sizeof(struct shared_memory), IPC_CREAT | 0666);
    struct shared_memory* shm = shmat(shm_id, NULL, 0);

    while (1) {
        // �����Ͱ� �غ�� ������ ���
        while (!shm->ready) {
            usleep(100); // CPU ��뷮�� ���̱� ���� ���
        }

        // ��� ����
        if (shm->client.op == '+') {
            shm->result.result = shm->client.left_num + shm->client.right_num;
        }
        else if (shm->client.op == '-') {
            shm->result.result = shm->client.left_num - shm->client.right_num;
        }
        else if (shm->client.op == 'x') {
            shm->result.result = shm->client.left_num * shm->client.right_num;
        }
        else if (shm->client.op == '/') {
            shm->result.result = (shm->client.right_num != 0) ? (shm->client.left_num / shm->client.right_num) : 0;
        }

        // min, max ������Ʈ
        if (shm->result.result < shm->result.min) {
            shm->result.min = shm->result.result;
        }
        if (shm->result.result > shm->result.max) {
            shm->result.max = shm->result.result;
        }

        // ���� �ð� ��������
        time_t rawtime = time(NULL);
        shm->result.timestamp = *localtime(&rawtime);

        // ������ IP �ּ� ����
        socklen_t addr_len = sizeof(shm->result.server_ip);
        getsockname(client_sock, (struct sockaddr*)&shm->result.server_ip, &addr_len);

        // Ŭ���̾�Ʈ���� ��� ����
        send(client_sock, &shm->result, sizeof(shm->result), 0);

        // ������ �غ� ���� �ʱ�ȭ
        shm->ready = 0;
    }

    shmdt(shm);
    return NULL;
}

int main() {
    int server_sock, client_sock;
    struct sockaddr_in server_addr, client_addr;
    pthread_t producer_tid, consumer_tid;

    // ���� �޸� �ʱ�ȭ
    int shm_id = shmget(SHM_KEY, sizeof(struct shared_memory), IPC_CREAT | 0666);
    struct shared_memory* shm = shmat(shm_id, NULL, 0);
    shm->ready = 0; // �ʱ� ����

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

        // ���ε༭�� ������ ������ ����
        int* pclient = malloc(sizeof(int));
        *pclient = client_sock;
        pthread_create(&producer_tid, NULL, producer, pclient);
        pthread_create(&consumer_tid, NULL, consumer, NULL);
    }

    // �ڿ� ����
    close(server_sock);
    shmdt(shm);
    return 0;
}
