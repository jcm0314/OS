#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <time.h>

#define PORT 8080

// 클라이언트 요청 구조체
struct client_data {
    int left_num;
    int right_num;
    char op; // 연산자
};

// 서버 응답 구조체
struct result_data {
    int result; // 결과
    struct tm timestamp; // 시간 정보
};

int main() {
    int server_sock, client_sock;
    struct sockaddr_in server_addr, client_addr;
    socklen_t client_len = sizeof(client_addr);
    struct client_data data;
    struct result_data result;

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
    listen(server_sock, 5);
    printf("Server is listening on port %d\n", PORT);

    while (1) {
        // 클라이언트 연결 수락
        client_sock = accept(server_sock, (struct sockaddr*)&client_addr, &client_len);
        if (client_sock < 0) {
            perror("accept failed");
            continue;
        }

        // 클라이언트 데이터 수신
        if (recv(client_sock, &data, sizeof(data), 0) <= 0) {
            perror("recv failed");
            close(client_sock);
            continue;
        }

        // 계산 수행
        switch (data.op) {
            case '+':
                result.result = data.left_num + data.right_num;
                break;
            case '-':
                result.result = data.left_num - data.right_num;
                break;
            case 'x':
                result.result = data.left_num * data.right_num;
                break;
            case '/':
                result.result = (data.right_num != 0) ? (data.left_num / data.right_num) : 0;
                break;
            default:
                result.result = 0; // 잘못된 연산자 처리
                break;
        }

        // 현재 시간 가져오기
        time_t rawtime = time(NULL);
        result.timestamp = *localtime(&rawtime);

        // 클라이언트에 결과 전송
        send(client_sock, &result, sizeof(result), 0);
        close(client_sock);
    }

    // 자원 정리
    close(server_sock);
    return 0;
}
