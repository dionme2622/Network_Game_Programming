#include "..\..\Common.h"

#define SERVERPORT 9000
#define BUFSIZE    512

int main()
{

    int retval;
    // ���� �ʱ�ȭ

    WSADATA wsa;
    if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) return 1;

    // ���� ����
    SOCKET listen_sock = socket(AF_INET, SOCK_STREAM, 0);
    if (listen_sock == INVALID_SOCKET) err_quit("socket()");


    // ���� �ּ� ���� �� ���ε�

    struct sockaddr_in serveraddr;
    memset(&serveraddr, 0, sizeof(serveraddr));
    serveraddr.sin_family = AF_INET;
    serveraddr.sin_addr.s_addr = htonl(INADDR_ANY);
    serveraddr.sin_port = htons(SERVERPORT);



    retval = bind(listen_sock, (struct sockaddr*)&serveraddr, sizeof(serveraddr));
    if (retval == SOCKET_ERROR) err_quit("bind()");

    // ���� ���·� ��ȯ

    retval = listen(listen_sock, SOMAXCONN);
    if (retval == SOCKET_ERROR) err_quit("listen()");

    printf("������ ���� ���� ��� ���Դϴ�...\n");

    while (1) {

        // Ŭ���̾�Ʈ ���� ����
        struct sockaddr_in clientaddr;
        int addrlen = sizeof(clientaddr);
        SOCKET client_sock = accept(listen_sock, (struct sockaddr*)&clientaddr, &addrlen);

        if (client_sock == INVALID_SOCKET) {
            err_display("accept()");
            break;
        }

        // Ŭ���̾�Ʈ ���� ���
        char addr[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &clientaddr.sin_addr, addr, sizeof(addr));
        printf("\n[TCP ����] Ŭ���̾�Ʈ ����: IP �ּ�=%s, ��Ʈ ��ȣ=%d\n",
            addr, ntohs(clientaddr.sin_port));

        // ���� �̸� �ޱ�

        int len;
        retval = recv(client_sock, (char*)&len, sizeof(int), MSG_WAITALL);  // ���� ���� ������ (���� �̸� ����)

        if (retval == SOCKET_ERROR) {
            err_display("recv()");
            break;
        }

        char filename[BUFSIZE];
        retval = recv(client_sock, filename, len, MSG_WAITALL);  // ���� �̸� ����

        if (retval == SOCKET_ERROR) {
            err_display("recv()");
            break;
        }
        filename[retval] = '\0';  // ���� �̸� ���� ���� �߰�

        // ���� ũ�� �ޱ�
        long filesize;

        retval = recv(client_sock, (char*)&filesize, sizeof(long), MSG_WAITALL);  // ���� ũ�� ����

        if (retval == SOCKET_ERROR) {
            err_display("recv()");
            break;
        }

        // ���� �̸����� ��� ����

        char* only_filename = strrchr(filename, '\\');  // ������ '\'�� ã��

        if (only_filename != NULL) {
            only_filename++;  // ��� ���� ���ϸ� �κ��� ���
        }
        else {
            only_filename = filename;  // ��ΰ� ������ �״�� ���
        }

        // ������ ������ ���� (���� ���)
        FILE* file = fopen(only_filename, "wb");
        if (file == NULL) {
            printf("���� ���⿡ �����߽��ϴ�: %s\n", only_filename);
            closesocket(client_sock);
            continue;
        }
        printf("[TCP ����] ���� ���� ����: %s\n", only_filename);

        // ���� ������ ����
        char buf[BUFSIZE];
        long total_bytes_received = 0; // �� ���� ����Ʈ ��
        while (1) {
            retval = recv(client_sock, buf, BUFSIZE, 0);  // ���� ������ ����
            if (retval == SOCKET_ERROR) {
                err_display("recv()");
                break;
            }
            else if (retval == 0) {
                break;  // ���� ����
            }
            fwrite(buf, 1, retval, file);  // ���Ͽ� ������ ����
            total_bytes_received += retval; // ���ŵ� ����Ʈ �� ������Ʈ

            // ������ ���
            printf("\r[������] (%.1f%%)", (double)total_bytes_received / filesize * 100);
            fflush(stdout); // ��� ���� ����
        }
        printf("[TCP ����] ���� ���� �Ϸ�: %s\n", only_filename);


        // ���� �� ���� �ݱ�
        fclose(file);
        closesocket(client_sock);

    }

    // ���� �ݱ� �� ���� ����
    closesocket(listen_sock);
    WSACleanup();
    return 0;
}