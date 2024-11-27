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
#include <limits.h>  // INT_MAX와 INT_MIN 사용을 위해 추가

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
    int min;
    int max;
    struct tm timestamp;
    struct sockaddr_in server_ip;
};

// 클라이언트에서 보내는 학생 정보와 계산 요청을 처리하는 함수
void* handle_client(void* arg) {
    int client_sock = *(int*)arg;
    free(arg);

    struct client_data data;
    struct result_data result;
    int shm_id = shmget(SHM_KEY, sizeof(struct result_data), IPC_CREAT | 0666);
    struct result_data* shm_result = shmat(shm_id, NULL, 0);

    // 초기 min, max 설정
    result.min = INT_MAX;
    result.max = INT_MIN;

    while (1) {
        // 클라이언트로부터 데이터 수신
        if (recv(client_sock, &data, sizeof(data), 0) <= 0) {
            perror("recv failed");
            break;
        }

        // 종료 조건
        if (data.left_num == 0 && data.right_num == 0 && data.op == '$') {
            break;
        }

        // 계산 수행
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

        // min, max 업데이트
        if (result.result < result.min) {
            result.min = result.result;
        }
        if (result.result > result.max) {
            result.max = result.result;
        }

        // 현재 시간 가져오기
        time_t rawtime = time(NULL);
        result.timestamp = *localtime(&rawtime);

        // 서버의 IP 주소 설정
        socklen_t addr_len = sizeof(result.server_ip);
        getsockname(client_sock, (struct sockaddr*)&result.server_ip, &addr_len);

        // 공유 메모리에 결과 저장
        memcpy(shm_result, &result, sizeof(result));

        // 클라이언트에게 결과 전송
        send(client_sock, shm_result, sizeof(result), 0);

        // 10초 대기
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

        // 클라이언트를 처리하는 스레드 생성
        int* pclient = malloc(sizeof(int));
        *pclient = client_sock;
        pthread_create(&tid, NULL, handle_client, pclient);
    }

    close(server_sock);
    return 0;
}
