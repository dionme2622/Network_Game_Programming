#include "..\..\..\..\Common.h"
#include "resource.h"
#include <shlobj.h> // SHBrowseForFolder �Լ� ����� ���� ����

#define SERVERIP   "127.0.0.1"
#define SERVERPORT 9000
#define BUFSIZE    512
#define MAX_BUTTON 2 

namespace BUTTON {
	enum { IP_INPUT = 0, FILE_OPEN = 1 };
}

// ��ȭ���� ���ν���
INT_PTR CALLBACK DlgProc(HWND, UINT, WPARAM, LPARAM);
// ���� ��� ������ �Լ�
DWORD WINAPI ClientMain(LPVOID arg);

SOCKET sock;				// ����
char buf[BUFSIZE + 1];		// ������ �ۼ��� ����
HWND hButton[MAX_BUTTON];	// ������ ��ư
HWND hEdit, hEdit2;			// ����Ʈ ��Ʈ��
HWND hProgress;				// ����� ��Ʈ��
char serverIP[16];			// ���� IP �ּ�
OPENFILENAMEA ofn;			// ���� ���� ����ü
char filename[MAX_PATH] = { 0 }; // ���� ��θ� ������ ����

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance,
	LPSTR lpCmdLine, int nCmdShow)
{
	// ���� �ʱ�ȭ
	WSADATA wsa;
	if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0)
		return 1;

	// ��ȭ���� ����
	DialogBox(hInstance, MAKEINTRESOURCE(IDD_DIALOG1), NULL, DlgProc);

	// ���� ����
	WSACleanup();
	return 0;
}

// ��ȭ���� ���ν���
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
			// ���� ��� ������ ����
			CreateThread(NULL, 0, ClientMain, NULL, 0, NULL);
			return TRUE;
			
		case IDC_FILE_OPEN:
			// TODO : ���丮 ���⸦ �����Ѵ�
			// ���� �̸��� �޾ƿ´�
			// OPENFILENAME ����ü �ʱ�ȭ
			ZeroMemory(&ofn, sizeof(ofn));
			ofn.lStructSize = sizeof(ofn);
			ofn.hwndOwner = hDlg;              // �θ� ������ �ڵ�
			ofn.lpstrFile = filename;            // ���õ� ���� ��θ� ���� ����
			ofn.nMaxFile = sizeof(filename);     // ���� ũ��
			ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;

			// ���� ���� ��ȭ ���� ����
			GetOpenFileNameA(&ofn);											// ���� ���丮�� ����.
			SendMessageA(hEdit2, EM_REPLACESEL, FALSE, (LPARAM)filename);  // ������ ������ ��θ� Edit Box�� ����Ѵ�.
			return TRUE;
		case IDCANCEL:
			EndDialog(hDlg, IDCANCEL); // ��ȭ���� �ݱ�
			closesocket(sock); // ���� �ݱ�
			return TRUE;
		}
		return FALSE;
	}
	return FALSE;
}

// TCP Ŭ���̾�Ʈ ���� �κ�
DWORD WINAPI ClientMain(LPVOID arg)
{

    int retval;
    // ���� ����
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

	retval = send(sock, (char*)&filesize, sizeof(long), 0);  // ���� ũ�� ����
	if (retval == SOCKET_ERROR) err_quit("send()");

	// ���� ������ ����
	long bytes_sent = 0;
	char buf[BUFSIZE];
	while (!feof(file)) {
		int bytes_read = fread(buf, 1, BUFSIZE, file);  // ���Ͽ��� ������ �б�
		if (bytes_read > 0) {
			retval = send(sock, buf, bytes_read, 0);  // ������ ����
			if (retval == SOCKET_ERROR) err_quit("send()");
			// 
		}
		bytes_sent += bytes_read;	

		int progress = (int)((bytes_sent * 100) / filesize); // ���۷��� �ۼ�Ʈ�� ���
		SendMessage(hProgress, PBM_SETPOS, progress, 0); // ����� ������Ʈ
	}
	MessageBoxA(NULL, "���� �۽ſϷ�", "", MB_OK);		// ���� �۽��� �Ϸ�Ǹ� �޽����� ������ 

    closesocket(sock); // ������ ����
    return 0;
}
	
