#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <time.h>

#define PORT 8080
#define MAX_STRING_LENGTH 50

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
    server_addr.sin_addr.s_addr = inet_addr("10.20.0.90"); // 로컬 서버
    server_addr.sin_port = htons(PORT);

    // 서버에 연결
    if (connect(sock, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        perror("connection failed");
        close(sock);
        exit(EXIT_FAILURE);
    }

    // 입력 반복
    while (1) {
        printf("Enter two integers, operator (e.g., +, -, x, /), and a string (or '0 0 $ quit' to exit): ");
        scanf("%d %d %c %s", &data.left_num, &data.right_num, &data.op, data.student_info);

        // 종료 조건
        if (data.left_num == 0 && data.right_num == 0 && data.op == '$') {
            send(sock, &data, sizeof(data), 0); // 종료 메시지 전송
            break;
        }

        // 클라이언트 데이터 전송
        send(sock, &data, sizeof(data), 0);

        // 서버로부터 결과 수신
        recv(sock, &result, sizeof(result), 0);
        
        // 결과 출력
        printf("Received from server: Result = %d, Min = %d, Max = %d, Time = %s, from %s\n",
               result.result, result.min, result.max, asctime(&result.timestamp),
               inet_ntoa(result.client_ip.sin_addr));
    }

    close(sock);
    return 0;
}
