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
#define SHM_KEY 1234 // 공유 메모리 키를 변경

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
};

// 공유 메모리 구조체
struct shared_memory {
    struct client_data client;
    struct result_data result;
    int ready; // 데이터 준비 상태 플래그
    int client_sock; // 클라이언트 소켓 추가
};

int main() {
    int server_sock, client_sock;
    struct sockaddr_in server_addr, client_addr;

    // 공유 메모리 크기 출력
    printf("Size of shared_memory: %lu bytes\n", sizeof(struct shared_memory));

    // 공유 메모리 초기화
    int shm_id = shmget(SHM_KEY, sizeof(struct shared_memory), IPC_CREAT | 0666);
    if (shm_id < 0) {
        perror("shmget failed");
        exit(EXIT_FAILURE);
    }

    struct shared_memory* shm = shmat(shm_id, NULL, 0);
    if (shm == (void*)-1) {
        perror("shmat failed");
        exit(EXIT_FAILURE);
    }
    
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
        // 스레드 생성 및 처리 코드 추가 필요
    }

    // 자원 정리
    close(server_sock);
    shmdt(shm);
    return 0;
}
