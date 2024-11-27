#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <time.h>

#define PORT 8080

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
    struct tm timestamp; // <time.h>에서 정의된 구조체
    struct sockaddr_in server_ip;
};

int main() {
    int sock;
    struct sockaddr_in server_addr;
    struct client_data data;
    struct result_data result;

    // 소켓 생성
    sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        perror("socket creation failed");
        exit(EXIT_FAILURE);
    }

    // 서버 주소 설정
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = inet_addr("127.0.0.1"); // 서버 IP 주소
    server_addr.sin_port = htons(PORT);

    // 서버에 연결
    if (connect(sock, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        perror("connection failed");
        close(sock);
        exit(EXIT_FAILURE);
    }

    // 학생 정보를 저장
    strcpy(data.student_info, "OSNW2024, 학번, 이름");

    while (1) {
        // 사용자 입력 받기
        printf("Enter two integers, operator (e.g., +, -, x, /): ");
        scanf("%d %d %c", &data.left_num, &data.right_num, &data.op);

        if (data.left_num == 0 && data.right_num == 0 && data.op == '$') {
            // 종료 조건
            data.left_num = 0;
            data.right_num = 0;
            data.op = '$';
            send(sock, &data, sizeof(data), 0);
            break;
        }

        // 클라이언트 데이터 전송
        send(sock, &data, sizeof(data), 0);

        // 서버로부터 결과 수신
        recv(sock, &result, sizeof(result), 0);
        printf("Received from server: Result = %d, Min = %d, Max = %d, Time = %s",
            result.result, result.min, result.max, asctime(&result.timestamp));
    }

    close(sock);
    return 0;
}
