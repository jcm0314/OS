#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <time.h>

#define PORT 8080

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
    struct tm timestamp; // <time.h>���� ���ǵ� ����ü
    struct sockaddr_in server_ip;
};

int main() {
    int sock;
    struct sockaddr_in server_addr;
    struct client_data data;
    struct result_data result;

    // ���� ����
    sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        perror("socket creation failed");
        exit(EXIT_FAILURE);
    }

    // ���� �ּ� ����
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = inet_addr("127.0.0.1"); // ���� IP �ּ�
    server_addr.sin_port = htons(PORT);

    // ������ ����
    if (connect(sock, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        perror("connection failed");
        close(sock);
        exit(EXIT_FAILURE);
    }

    // �л� ������ ����
    strcpy(data.student_info, "OSNW2024, �й�, �̸�");

    while (1) {
        // ����� �Է� �ޱ�
        printf("Enter two integers, operator (e.g., +, -, x, /): ");
        scanf("%d %d %c", &data.left_num, &data.right_num, &data.op);

        if (data.left_num == 0 && data.right_num == 0 && data.op == '$') {
            // ���� ����
            data.left_num = 0;
            data.right_num = 0;
            data.op = '$';
            send(sock, &data, sizeof(data), 0);
            break;
        }

        // Ŭ���̾�Ʈ ������ ����
        send(sock, &data, sizeof(data), 0);

        // �����κ��� ��� ����
        recv(sock, &result, sizeof(result), 0);
        printf("Received from server: Result = %d, Min = %d, Max = %d, Time = %s",
            result.result, result.min, result.max, asctime(&result.timestamp));
    }

    close(sock);
    return 0;
}