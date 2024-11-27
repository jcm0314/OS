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
#define MAX_STRING_LENGTH 50
#define MAX_CLIENTS 3

// 클라이언트 요청 구조체
struct client_data {
    int left_num;
    int right_num;
    char op; // 연산자
    char student_info[MAX_STRING_LENGTH]; // 문자열
};

// 서버 응답 구조체
struct result_data {
    int result; // 결과
    int min;    // 최소값
    int max;    // 최대값
    struct tm timestamp; // 시간 정보
    struct sockaddr_in client_ip; // 클라이언트 IP 정보
};

// 공유 메모리 구조체
struct shared_memory {
    struct result_data result;
};

void* handle_client(void* arg) {
    int client_sock = *(int*)arg;
    struct shared_memory* shm;
    int shm_id;

    // 공유 메모리 생성
    shm_id = shmget(IPC_PRIVATE, sizeof(struct shared_memory), IPC_CREAT | 0666);
    shm = (struct shared_memory*)shmat(shm_id, NULL, 0);

    // 초기값 설정
    shm->result.min = INT_MAX;
    shm->result.max = INT_MIN;

    while (1) {
        struct client_data data;

        // 클라이언트 데이터 수신
        if (recv(client_sock, &data, sizeof(data), 0) <= 0) {
            perror("recv failed");
            break;
        }

        // 종료 조건
        if (data.left_num == 0 && data.right_num == 0 && data.op == '$') {
            break;
        }

        // 계산 수행
        switch (data.op) {
            case '+':
                shm->result.result = data.left_num + data.right_num;
                break;
            case '-':
                shm->result.result = data.left_num - data.right_num;
                break;
            case 'x':
                shm->result.result = data.left_num * data.right_num;
                break;
            case '/':
                shm->result.result = (data.right_num != 0) ? (data.left_num / data.right_num) : 0;
                break;
            default:
                shm->result.result = 0; // 잘못된 연산자 처리
                break;
        }

        // min, max 업데이트
        if (shm->result.result < shm->result.min) {
            shm->result.min = shm->result.result;
        }
        if (shm->result.result > shm->result.max) {
            shm->result.max = shm->result.result;
        }

        // 현재 시간 가져오기
        time_t rawtime = time(NULL);
        shm->result.timestamp = *localtime(&rawtime);
        shm->result.client_ip = *(struct sockaddr_in*)&client_sock;

        // 10초 간격으로 클라이언트에 결과 전송
        send(client_sock, &shm->result, sizeof(shm->result), 0);
        sleep(10);
    }

    // 자원 정리
    shmdt(shm);
    close(client_sock);
    return NULL;
}

int main() {
    int server_sock, client_sock;
    struct sockaddr_in server_addr, client_addr;
    socklen_t client_len = sizeof(client_addr);
    pthread_t threads[MAX_CLIENTS];

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

    for (int i = 0; i < MAX_CLIENTS; i++) {
        // 클라이언트 연결 수락
        client_sock = accept(server_sock, (struct sockaddr*)&client_addr, &client_len);
        if (client_sock < 0) {
            perror("accept failed");
            continue;
        }

        // 새로운 스레드를 생성하여 클라이언트 처리
        if (pthread_create(&threads[i], NULL, handle_client, &client_sock) != 0) {
            perror("pthread_create failed");
        }
    }

    // 모든 스레드 종료 대기
    for (int i = 0; i < MAX_CLIENTS; i++) {
        pthread_join(threads[i], NULL);
    }

    // 자원 정리
    close(server_sock);
    return 0;
}
