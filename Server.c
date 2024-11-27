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

// 클라이언트와 서버 간의 데이터 구조체
struct client_data {
    int left_num;
    int right_num;
    char op;
    char student_info[50]; // OSNW2024, 학번, 이름 등의 고정 문자열
};

struct result_data {
    int result;
    int min; // 서버에서 계산된 min
    int max; // 최대값
    struct tm timestamp; // <time.h>에서 정의된 구조체
    struct sockaddr_in server_ip;
};

// 공유 메모리 구조체
struct shared_memory {
    struct client_data client;
    struct result_data result;
    int ready; // 데이터 준비 상태 플래그
    int client_sock; // 클라이언트 소켓 추가
};

// 프로듀서 함수
void* producer(void* arg) {
    int client_sock = *(int*)arg;
    free(arg);

    int shm_id = shmget(SHM_KEY, sizeof(struct shared_memory), IPC_CREAT | 0666);
    struct shared_memory* shm = shmat(shm_id, NULL, 0);

    shm->client_sock = client_sock; // 클라이언트 소켓 저장

    while (1) {
        // 클라이언트로부터 데이터 수신
        if (recv(client_sock, &shm->client, sizeof(shm->client), 0) <= 0) {
            perror("recv failed");
            break;
        }

        // 종료 조건
        if (shm->client.left_num == 0 && shm->client.right_num == 0 && shm->client.op == '$') {
            break;
        }

        // 데이터 준비 상태 설정
        shm->ready = 1;
    }

    close(client_sock);
    shmdt(shm);
    return NULL;
}

// 컨슈머 함수
void* consumer(void* arg) {
    int shm_id = shmget(SHM_KEY, sizeof(struct shared_memory), IPC_CREAT | 0666);
    struct shared_memory* shm = shmat(shm_id, NULL, 0);

    while (1) {
        // 데이터가 준비될 때까지 대기
        while (!shm->ready) {
            usleep(100); // CPU 사용량을 줄이기 위해 대기
        }

        // 계산 수행
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

        // min, max 업데이트
        if (shm->result.result < shm->result.min || shm->result.min == 0) {
            shm->result.min = shm->result.result;
        }
        if (shm->result.result > shm->result.max) {
            shm->result.max = shm->result.result;
        }

        // 현재 시간 가져오기
        time_t rawtime = time(NULL);
        shm->result.timestamp = *localtime(&rawtime);

        // 클라이언트 소켓으로 결과 전송
        send(shm->client_sock, &shm->result, sizeof(shm->result), 0);

        // 데이터 준비 상태 초기화
        shm->ready = 0;
    }

    shmdt(shm);
    return NULL;
}

int main() {
    int server_sock, client_sock;
    struct sockaddr_in server_addr, client_addr;
    pthread_t producer_tid, consumer_tid;

    // 공유 메모리 초기화
    int shm_id = shmget(SHM_KEY, sizeof(struct shared_memory), IPC_CREAT | 0666);
    struct shared_memory* shm = shmat(shm_id, NULL, 0);
    shm->ready = 0; // 초기 상태

    // 소켓 생성
    server_sock = socket(AF_INET, SOCK_STREAM, 0);
    if (server_sock < 0) {
        perror("socket creation failed");
        exit(EXIT_FAILURE);
    }

    // 서버 주소 설정
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(PORT);

    // 바인드
    if (bind(server_sock, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        perror("bind failed");
        close(server_sock);
        exit(EXIT_FAILURE);
    }

    // 리스닝
    listen(server_sock, MAX_CLIENTS);
    printf("Server is listening on port %d\n", PORT);

    while (1) {
        socklen_t client_len = sizeof(client_addr);
        // 클라이언트 연결 수락
        client_sock = accept(server_sock, (struct sockaddr*)&client_addr, &client_len);
        if (client_sock < 0) {
            perror("accept failed");
            continue;
        }

        // 프로듀서와 컨슈머 스레드 생성
        int* pclient = malloc(sizeof(int));
        *pclient = client_sock;
        pthread_create(&producer_tid, NULL, producer, pclient);
        pthread_create(&consumer_tid, NULL, consumer, NULL);
    }

    // 자원 정리
    close(server_sock);
    shmdt(shm);
    return 0;
}
