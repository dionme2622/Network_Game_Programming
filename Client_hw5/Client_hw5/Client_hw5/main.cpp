#include "..\..\..\..\Common.h"
#include "resource.h"
#include <shlobj.h> // SHBrowseForFolder 함수 사용을 위해 포함

#define SERVERIP   "127.0.0.1"
#define SERVERPORT 9000
#define BUFSIZE    512
#define MAX_BUTTON 2 

namespace BUTTON {
	enum { IP_INPUT = 0, FILE_OPEN = 1 };
}

// 대화상자 프로시저
INT_PTR CALLBACK DlgProc(HWND, UINT, WPARAM, LPARAM);
// 소켓 통신 스레드 함수
DWORD WINAPI ClientMain(LPVOID arg);

SOCKET sock;				// 소켓
char buf[BUFSIZE + 1];		// 데이터 송수신 버퍼
HWND hButton[MAX_BUTTON];	// 보내기 버튼
HWND hEdit, hEdit2;			// 에디트 컨트롤
HWND hProgress;				// 진행률 컨트롤
char serverIP[16];			// 서버 IP 주소
OPENFILENAMEA ofn;			// 파일 열기 구조체
char filename[MAX_PATH] = { 0 }; // 파일 경로를 저장할 버퍼

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance,
	LPSTR lpCmdLine, int nCmdShow)
{
	// 윈속 초기화
	WSADATA wsa;
	if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0)
		return 1;

	// 대화상자 생성
	DialogBox(hInstance, MAKEINTRESOURCE(IDD_DIALOG1), NULL, DlgProc);

	// 윈속 종료
	WSACleanup();
	return 0;
}

// 대화상자 프로시저
INT_PTR CALLBACK DlgProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg) {
	case WM_INITDIALOG:
		hEdit = GetDlgItem(hDlg, IDC_IP_EDIT);
		hEdit2 = GetDlgItem(hDlg, IDC_EDIT1);
		hButton[BUTTON::IP_INPUT] = GetDlgItem(hDlg, IDC_INPUT_IP);
		hButton[BUTTON::FILE_OPEN] = GetDlgItem(hDlg, IDC_FILE_OPEN);
		hProgress = GetDlgItem(hDlg, IDC_PROGRESS1);
		SendMessage(hEdit, EM_SETLIMITTEXT, BUFSIZE, 0);
		return TRUE;
	case WM_COMMAND:
		switch (LOWORD(wParam)) {
		case IDC_INPUT_IP:
			GetDlgItemTextA(hDlg, IDC_IP_EDIT, serverIP, sizeof(serverIP));
			// 소켓 통신 스레드 생성
			CreateThread(NULL, 0, ClientMain, NULL, 0, NULL);
			return TRUE;
			
		case IDC_FILE_OPEN:
			// TODO : 디렉토리 열기를 실행한다
			// 파일 이름을 받아온다
			// OPENFILENAME 구조체 초기화
			ZeroMemory(&ofn, sizeof(ofn));
			ofn.lStructSize = sizeof(ofn);
			ofn.hwndOwner = hDlg;              // 부모 윈도우 핸들
			ofn.lpstrFile = filename;            // 선택된 파일 경로를 받을 버퍼
			ofn.nMaxFile = sizeof(filename);     // 버퍼 크기
			ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;

			// 파일 열기 대화 상자 실행
			GetOpenFileNameA(&ofn);											// 파일 디렉토리를 연다.
			SendMessageA(hEdit2, EM_REPLACESEL, FALSE, (LPARAM)filename);  // 선택한 파일의 경로를 Edit Box에 출력한다.
			return TRUE;
		case IDCANCEL:
			EndDialog(hDlg, IDCANCEL); // 대화상자 닫기
			closesocket(sock); // 소켓 닫기
			return TRUE;
		}
		return FALSE;
	}
	return FALSE;
}

// TCP 클라이언트 시작 부분
DWORD WINAPI ClientMain(LPVOID arg)
{

    int retval;
    // 소켓 생성
    sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock == INVALID_SOCKET) err_quit("socket()");

    // connect()
    struct sockaddr_in serveraddr;
    memset(&serveraddr, 0, sizeof(serveraddr));
    serveraddr.sin_family = AF_INET;
    serveraddr.sin_addr.s_addr = inet_addr(serverIP);
    serveraddr.sin_port = htons(SERVERPORT);
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

	retval = send(sock, (char*)&filesize, sizeof(long), 0);  // 파일 크기 전송
	if (retval == SOCKET_ERROR) err_quit("send()");

	// 파일 데이터 전송
	long bytes_sent = 0;
	char buf[BUFSIZE];
	while (!feof(file)) {
		int bytes_read = fread(buf, 1, BUFSIZE, file);  // 파일에서 데이터 읽기
		if (bytes_read > 0) {
			retval = send(sock, buf, bytes_read, 0);  // 서버로 전송
			if (retval == SOCKET_ERROR) err_quit("send()");
			// 
		}
		bytes_sent += bytes_read;	

		int progress = (int)((bytes_sent * 100) / filesize); // 전송률을 퍼센트로 계산
		SendMessage(hProgress, PBM_SETPOS, progress, 0); // 진행률 업데이트
	}
	MessageBoxA(NULL, "파일 송신완료", "", MB_OK);		// 파일 송신이 완료되면 메시지를 보낸다 

    closesocket(sock); // 소켓을 닫음
    return 0;
}
	
