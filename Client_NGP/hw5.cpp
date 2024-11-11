#include "..\..\Common.h"
#include "resource.h"

#define SERVERIP   "127.0.0.1"
#define SERVERPORT 9000
#define BUFSIZE    512

// 대화상자 프로시저
INT_PTR CALLBACK DlgProc(HWND, UINT, WPARAM, LPARAM);
// 에디트 컨트롤 출력 함수
void DisplayText(const char* fmt, ...);
// 소켓 함수 오류 출력
void DisplayError(const char* msg);
// 소켓 통신 스레드 함수
DWORD WINAPI ClientMain(LPVOID arg);

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance,
    LPSTR lpCmdLine, int nCmdShow)
{
    // 윈속 초기화
    WSADATA wsa;
    if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0)
        return 1;

    // 이벤트 생성
    hReadEvent = CreateEvent(NULL, FALSE, TRUE, NULL);
    hWriteEvent = CreateEvent(NULL, FALSE, FALSE, NULL);

    // 소켓 통신 스레드 생성
    CreateThread(NULL, 0, ClientMain, NULL, 0, NULL);

    // 대화상자 생성
    DialogBox(hInstance, MAKEINTRESOURCE(IDD_DIALOG1), NULL, DlgProc);

    // 이벤트 제거
    CloseHandle(hReadEvent);
    CloseHandle(hWriteEvent);

    // 윈속 종료
    WSACleanup();
    return 0;
}


int main(int argc, char* argv[])
{
    int retval;

    // 명령행 인수로 IP를 받을 수 있도록 수정
    if (argc > 2) {
        SERVERIP = argv[1];  // 첫 번째 인수로 서버 IP를 받음
    }

    // 파일 이름 입력 받기
    char filename[BUFSIZE];
    if (argc > 2) {
        strncpy(filename, argv[2], BUFSIZE - 1); // 두 번째 인수로 파일 이름을 받음
        filename[BUFSIZE - 1] = '\0'; // NULL 종료
    }
    else {
        printf("다시 입력하세요\n");
        return 1;
    }

    // 윈속 초기화

    WSADATA wsa;
    if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) return 1;

    // 소켓 생성
    SOCKET sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock == INVALID_SOCKET) err_quit("socket()");

    // 서버 주소 설정
    struct sockaddr_in serveraddr;
    memset(&serveraddr, 0, sizeof(serveraddr));
    serveraddr.sin_family = AF_INET;
    serveraddr.sin_addr.s_addr = inet_addr(SERVERIP);
    serveraddr.sin_port = htons(SERVERPORT);

    // 서버에 연결
    retval = connect(sock, (struct sockaddr*)&serveraddr, sizeof(serveraddr));
    if (retval == SOCKET_ERROR) err_quit("connect()");

    // 파일 열기

    FILE* file = fopen(filename, "rb");

    if (file == NULL) {
        printf("파일 열기에 실패했습니다: %s\n", filename);
        closesocket(sock);
        WSACleanup();
        return 1;
    }

    // 파일 크기 측정
    fseek(file, 0, SEEK_END);
    long filesize = ftell(file);
    fseek(file, 0, SEEK_SET);

    // 파일 이름 전송

    int len = (int)strlen(filename);  // 파일 이름 길이

    retval = send(sock, (char*)&len, sizeof(int), 0);  // 고정 길이 데이터 (파일 이름 길이)
    if (retval == SOCKET_ERROR)
        err_quit("send()");

    retval = send(sock, filename, len, 0);  // 가변 길이 데이터
    if (retval == SOCKET_ERROR)
        err_quit("send()");

    // 파일 크기 전송

    retval = send(sock, (char*)&filesize, sizeof(long), 0);  // 파일 크기 전송
    if (retval == SOCKET_ERROR) err_quit("send()");

    // 파일 데이터 전송

    char buf[BUFSIZE];
    while (!feof(file)) {
        int bytes_read = fread(buf, 1, BUFSIZE, file);  // 파일에서 데이터 읽기
        if (bytes_read > 0) {
            retval = send(sock, buf, bytes_read, 0);  // 서버로 전송
            if (retval == SOCKET_ERROR) err_quit("send()");
        }
    }

    printf("[TCP 클라이언트] 파일 전송이 완료되었습니다: %s\n", filename);

    // 소켓 및 파일 닫기
    fclose(file);
    closesocket(sock);
    WSACleanup();
    return 0;
}