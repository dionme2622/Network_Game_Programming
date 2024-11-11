#include "..\..\Common.h"
#include "resource.h"

#define SERVERIP   "127.0.0.1"
#define SERVERPORT 9000
#define BUFSIZE    512

// ��ȭ���� ���ν���
INT_PTR CALLBACK DlgProc(HWND, UINT, WPARAM, LPARAM);
// ����Ʈ ��Ʈ�� ��� �Լ�
void DisplayText(const char* fmt, ...);
// ���� �Լ� ���� ���
void DisplayError(const char* msg);
// ���� ��� ������ �Լ�
DWORD WINAPI ClientMain(LPVOID arg);

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance,
    LPSTR lpCmdLine, int nCmdShow)
{
    // ���� �ʱ�ȭ
    WSADATA wsa;
    if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0)
        return 1;

    // �̺�Ʈ ����
    hReadEvent = CreateEvent(NULL, FALSE, TRUE, NULL);
    hWriteEvent = CreateEvent(NULL, FALSE, FALSE, NULL);

    // ���� ��� ������ ����
    CreateThread(NULL, 0, ClientMain, NULL, 0, NULL);

    // ��ȭ���� ����
    DialogBox(hInstance, MAKEINTRESOURCE(IDD_DIALOG1), NULL, DlgProc);

    // �̺�Ʈ ����
    CloseHandle(hReadEvent);
    CloseHandle(hWriteEvent);

    // ���� ����
    WSACleanup();
    return 0;
}


int main(int argc, char* argv[])
{
    int retval;

    // ����� �μ��� IP�� ���� �� �ֵ��� ����
    if (argc > 2) {
        SERVERIP = argv[1];  // ù ��° �μ��� ���� IP�� ����
    }

    // ���� �̸� �Է� �ޱ�
    char filename[BUFSIZE];
    if (argc > 2) {
        strncpy(filename, argv[2], BUFSIZE - 1); // �� ��° �μ��� ���� �̸��� ����
        filename[BUFSIZE - 1] = '\0'; // NULL ����
    }
    else {
        printf("�ٽ� �Է��ϼ���\n");
        return 1;
    }

    // ���� �ʱ�ȭ

    WSADATA wsa;
    if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) return 1;

    // ���� ����
    SOCKET sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock == INVALID_SOCKET) err_quit("socket()");

    // ���� �ּ� ����
    struct sockaddr_in serveraddr;
    memset(&serveraddr, 0, sizeof(serveraddr));
    serveraddr.sin_family = AF_INET;
    serveraddr.sin_addr.s_addr = inet_addr(SERVERIP);
    serveraddr.sin_port = htons(SERVERPORT);

    // ������ ����
    retval = connect(sock, (struct sockaddr*)&serveraddr, sizeof(serveraddr));
    if (retval == SOCKET_ERROR) err_quit("connect()");

    // ���� ����

    FILE* file = fopen(filename, "rb");

    if (file == NULL) {
        printf("���� ���⿡ �����߽��ϴ�: %s\n", filename);
        closesocket(sock);
        WSACleanup();
        return 1;
    }

    // ���� ũ�� ����
    fseek(file, 0, SEEK_END);
    long filesize = ftell(file);
    fseek(file, 0, SEEK_SET);

    // ���� �̸� ����

    int len = (int)strlen(filename);  // ���� �̸� ����

    retval = send(sock, (char*)&len, sizeof(int), 0);  // ���� ���� ������ (���� �̸� ����)
    if (retval == SOCKET_ERROR)
        err_quit("send()");

    retval = send(sock, filename, len, 0);  // ���� ���� ������
    if (retval == SOCKET_ERROR)
        err_quit("send()");

    // ���� ũ�� ����

    retval = send(sock, (char*)&filesize, sizeof(long), 0);  // ���� ũ�� ����
    if (retval == SOCKET_ERROR) err_quit("send()");

    // ���� ������ ����

    char buf[BUFSIZE];
    while (!feof(file)) {
        int bytes_read = fread(buf, 1, BUFSIZE, file);  // ���Ͽ��� ������ �б�
        if (bytes_read > 0) {
            retval = send(sock, buf, bytes_read, 0);  // ������ ����
            if (retval == SOCKET_ERROR) err_quit("send()");
        }
    }

    printf("[TCP Ŭ���̾�Ʈ] ���� ������ �Ϸ�Ǿ����ϴ�: %s\n", filename);

    // ���� �� ���� �ݱ�
    fclose(file);
    closesocket(sock);
    WSACleanup();
    return 0;
}