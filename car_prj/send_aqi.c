#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

#define SERVER_IP "192.168.51.70" // 替換為 Pico W 的 IP 地址
#define SERVER_PORT 80            // Pico W 端口
#define UPDATE_INTERVAL 1         // 資料傳送間隔（秒）

// 模擬氣體濃度數據（可以根據實際數據進行更新）
float co2_concentration = 813.60;  // CO2 濃度（ppm）
float nh3_concentration = 542.40;  // NH3 濃度（ppm）
float alcohol_concentration = 339.00; // Alcohol 濃度（ppm）

void send_data_to_pico(const char *data) {
    int sock;
    struct sockaddr_in server_addr;
    char buffer[1024];

    // 建立套接字
    sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        perror("Socket creation failed");
        exit(1);
    }

    // 設定伺服器地址
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(SERVER_PORT);
    server_addr.sin_addr.s_addr = inet_addr(SERVER_IP);

    // 連接伺服器
    if (connect(sock, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("Connection failed");
        exit(1);
    }

    // 送資料
    send(sock, data, strlen(data), 0);

    // 顯示資料
    printf("Sent: %s\n", data);

    // 接收伺服器回應
    int bytes_received = recv(sock, buffer, sizeof(buffer) - 1, 0);
    if (bytes_received > 0) {
        buffer[bytes_received] = '\0';
        printf("Received: %s\n", buffer);
    } else {
        printf("Failed to receive response\n");
    }

    // 關閉套接字
    close(sock);
}

int main() {
    // 持續傳送資料
    while (1) {
        // 顯示氣體濃度
        printf("CO2: %.2f ppm\n", co2_concentration);
        printf("NH3: %.2f ppm\n", nh3_concentration);
        printf("Alcohol: %.2f ppm\n", alcohol_concentration);

        // 構建要傳送的資料
        char data[256];
        snprintf(data, sizeof(data), "CO2=%.2f ppm, NH3=%.2f ppm, Alcohol=%.2f ppm",
                 co2_concentration, nh3_concentration, alcohol_concentration);

        // 傳送資料到 Pico W
        send_data_to_pico(data);

        // 根據 CO2 濃度控制燈顏色
        if (co2_concentration > 1000) {
            printf("CO2 concentration is high. Turn on RED light.\n");
        } else if (co2_concentration > 500) {
            printf("CO2 concentration is moderate. Turn on YELLOW light.\n");
        } else {
            printf("CO2 concentration is low. Turn on GREEN light.\n");
        }

        // 延遲一段時間後再發送資料
        sleep(UPDATE_INTERVAL);
    }

    return 0;
}

