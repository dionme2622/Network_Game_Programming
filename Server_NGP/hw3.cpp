#include "..\..\Common.h"

#define SERVERPORT 9000
#define BUFSIZE    512

int main()
{

    int retval;
    // 윈속 초기화

    WSADATA wsa;
    if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) return 1;

    // 소켓 생성
    SOCKET listen_sock = socket(AF_INET, SOCK_STREAM, 0);
    if (listen_sock == INVALID_SOCKET) err_quit("socket()");


    // 서버 주소 설정 및 바인딩

    struct sockaddr_in serveraddr;
    memset(&serveraddr, 0, sizeof(serveraddr));
    serveraddr.sin_family = AF_INET;
    serveraddr.sin_addr.s_addr = htonl(INADDR_ANY);
    serveraddr.sin_port = htons(SERVERPORT);



    retval = bind(listen_sock, (struct sockaddr*)&serveraddr, sizeof(serveraddr));
    if (retval == SOCKET_ERROR) err_quit("bind()");

    // 리슨 상태로 전환

    retval = listen(listen_sock, SOMAXCONN);
    if (retval == SOCKET_ERROR) err_quit("listen()");

    printf("서버가 파일 수신 대기 중입니다...\n");

    while (1) {

        // 클라이언트 연결 수락
        struct sockaddr_in clientaddr;
        int addrlen = sizeof(clientaddr);
        SOCKET client_sock = accept(listen_sock, (struct sockaddr*)&clientaddr, &addrlen);

        if (client_sock == INVALID_SOCKET) {
            err_display("accept()");
            break;
        }

        // 클라이언트 정보 출력
        char addr[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &clientaddr.sin_addr, addr, sizeof(addr));
        printf("\n[TCP 서버] 클라이언트 접속: IP 주소=%s, 포트 번호=%d\n",
            addr, ntohs(clientaddr.sin_port));

        // 파일 이름 받기

        int len;
        retval = recv(client_sock, (char*)&len, sizeof(int), MSG_WAITALL);  // 고정 길이 데이터 (파일 이름 길이)

        if (retval == SOCKET_ERROR) {
            err_display("recv()");
            break;
        }

        char filename[BUFSIZE];
        retval = recv(client_sock, filename, len, MSG_WAITALL);  // 파일 이름 수신

        if (retval == SOCKET_ERROR) {
            err_display("recv()");
            break;
        }
        filename[retval] = '\0';  // 파일 이름 종료 문자 추가

        // 파일 크기 받기
        long filesize;

        retval = recv(client_sock, (char*)&filesize, sizeof(long), MSG_WAITALL);  // 파일 크기 수신

        if (retval == SOCKET_ERROR) {
            err_display("recv()");
            break;
        }

        // 파일 이름에서 경로 제거

        char* only_filename = strrchr(filename, '\\');  // 마지막 '\'를 찾음

        if (only_filename != NULL) {
            only_filename++;  // 경로 다음 파일명 부분을 사용
        }
        else {
            only_filename = filename;  // 경로가 없으면 그대로 사용
        }

        // 수신한 파일을 열기 (쓰기 모드)
        FILE* file = fopen(only_filename, "wb");
        if (file == NULL) {
            printf("파일 열기에 실패했습니다: %s\n", only_filename);
            closesocket(client_sock);
            continue;
        }
        printf("[TCP 서버] 파일 저장 시작: %s\n", only_filename);

        // 파일 데이터 수신
        char buf[BUFSIZE];
        long total_bytes_received = 0; // 총 수신 바이트 수
        while (1) {
            retval = recv(client_sock, buf, BUFSIZE, 0);  // 파일 데이터 수신
            if (retval == SOCKET_ERROR) {
                err_display("recv()");
                break;
            }
            else if (retval == 0) {
                break;  // 전송 종료
            }
            fwrite(buf, 1, retval, file);  // 파일에 데이터 쓰기
            total_bytes_received += retval; // 수신된 바이트 수 업데이트

            // 수신율 출력
            printf("\r[수신중] (%.1f%%)", (double)total_bytes_received / filesize * 100);
            fflush(stdout); // 출력 버퍼 비우기
        }
        printf("[TCP 서버] 파일 수신 완료: %s\n", only_filename);


        // 소켓 및 파일 닫기
        fclose(file);
        closesocket(client_sock);

    }

    // 소켓 닫기 및 윈속 종료
    closesocket(listen_sock);
    WSACleanup();
    return 0;
}