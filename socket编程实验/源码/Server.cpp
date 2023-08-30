#pragma once
#include <Winsock2.h>
#include <stdio.h>
#include <iostream>
#include <string.h>
using namespace std;

#pragma comment(lib,"ws2_32.lib")

void send_head(SOCKET s, char* filename)//����ͷ��
{
	const char* content_type = "text/plain";
	char* extension;
	if ((extension = strrchr(filename, '.')) != NULL)
	{
		extension = extension + 1;	//�����ļ�����
	}
	else
	{
		printf("�����ļ�����ʧ�ܣ�\n");
		return;
	}

	if (!strcmp(extension, "html"))
		content_type = "text/html";
	else if (!strcmp(extension, "gif"))
		content_type = "image/gif";
	else if (!strcmp(extension, "jpg"))
		content_type = "image/jpg";
	else if (!strcmp(extension, "png"))
		content_type = "image/png";
	else if (strcmp(extension, "ico"))
	{
		printf("�����ļ�����ʧ�ܣ�\n");
		return;
	}

	const char* head = "HTTP/1.1 200 OK\r\n";//��Ӧ����ͷ��
	const char* not_find = "HTTP/1.1 404 NOT FOUND\r\n";
	int len, len1;
	char temp[30] = "Content-type: ";
	len = strlen(head);
	len1 = strlen(not_find);
	FILE* pfile = fopen(filename, "rb");
	if (pfile == NULL)
	{
		printf("Sending head: file not found!\n");
		send(s, not_find, len1, 0);
	}
	else if (send(s, head, len, 0) == -1)
	{
		printf("Sending head error");
		return;
	}
	if (content_type)
	{
		strcat(temp, content_type);
		strcat(temp, "\r\n");
		len = strlen(temp);

		if (send(s, temp, len, 0) == -1)
		{
			printf("Sending head error!");
			return;
		}
	}
	send(s, "\r\n", 2, 0);
}

bool send_file(SOCKET s, char* filename, char* dir)//�����ļ����ͻ���
{
	FILE* file = fopen(filename, "rb");
	if (file == NULL)
	{
		char tmp[100] = "";
		strcpy(tmp, dir);
		strcat(tmp, "404.html");
		file = fopen(tmp, "rb");
		if (file == NULL) {
			printf("��Ŀ¼·������!\n");
			return false;
		}
		else {
			printf("Sending file: file not found!\n\n");
		}
	}
	else
	{
		printf("Sending file: OK!\n\n");
	}

	fseek(file, 0L, SEEK_END);
	int flen = ftell(file);
	char* p = (char*)malloc(flen + 1);
	fseek(file, 0L, SEEK_SET);
	fread(p, flen, 1, file);
	send(s, p, flen, 0);
	return true;
}

int main() {
	WSADATA wsaData;
	int post;
	char adr[20];
	char dir[100];
	cout << "������˿ڣ�" << endl;
	cin >> post;
	cout << "�����������ַ��" << endl;
	cin >> adr;
	cout << "��������Ŀ¼��" << endl;
	cin >> dir;
	if (dir[strlen(dir) - 1] != '\\') {
		strcat(dir, "\\");
	}

	/*ini*/
	int nRc = WSAStartup(0x0202, &wsaData);
	if (nRc) 
	{
		printf("Winsock  startup failed with error!\n");
	}
	if (wsaData.wVersion != 0x0202) 
	{
		printf("Winsock version is not correct!\n");
	}
	printf("Winsock  startup Ok!\n");
	/*ini*/

	SOCKET srvSocket;
	sockaddr_in addr, clientAddr;
	SOCKET sessionSocket;
	int addrLen;
	
	/*socket*/
	srvSocket = socket(AF_INET, SOCK_STREAM, 0);
	if (srvSocket != INVALID_SOCKET)
		printf("Socket create Ok!\n");
	else printf("Socket create Error!\n");

	//set port and ip
	addr.sin_family = AF_INET;
	addr.sin_port = htons(post);
	addr.sin_addr.S_un.S_addr = inet_addr(adr);
	//addr.sin_addr.S_un.S_addr = htonl(INADDR_ANY);

	/*bind*/
	int rtn = bind(srvSocket, (LPSOCKADDR)&addr, sizeof(addr));
	if (rtn != SOCKET_ERROR)
		printf("Socket bind Ok!\n");
	else printf("Socket bind Error!\n");

	/*listen*/
	rtn = listen(srvSocket, 5);
	if (rtn != SOCKET_ERROR)
		printf("Socket listen Ok!\n");
	else printf("Socket listen Error!\n");

	addrLen = sizeof(clientAddr);
	char recvBuf[4096];
	cout << "ioctlsocket() for server socket ok!	Waiting for client connection and data\n";

	while (true) {
		/*accept*/
		sessionSocket = accept(srvSocket, (LPSOCKADDR)&clientAddr, &addrLen);
		if (sessionSocket != INVALID_SOCKET)
			printf("Socket listen one client request!\n");
		cout << "ioctlsocket() for session socket ok!	Waiting for client connection and data" << endl;

		/*receive*/
		memset(recvBuf, '\0', 4096);
		rtn = recv(sessionSocket, recvBuf, 1024, 0);
		printf("\nReceived %d bytes from client:\n%s", rtn, recvBuf);

		const char* name;
		name = strtok(recvBuf, "/");
		name = strtok(NULL, " ");
		char filename[100] = "";
		strcpy(filename, dir);
		strcat(filename, name);

		printf("�ͻ��˵�IP�Ͷ˿ںţ�%s:%d\n", inet_ntoa(clientAddr.sin_addr), htons(clientAddr.sin_port));
		printf("path:%s\n",filename);

		send_head(sessionSocket, filename);
		if (!send_file(sessionSocket, filename, dir)) {
			cout << "��������ȷ����Ŀ¼·����" << endl;
			cin >> dir;
			if (dir[strlen(dir) - 1] != '\\') {
				strcat(dir, "\\");
			}
		}

		closesocket(sessionSocket);  //��Ȼclient�뿪�ˣ��͹ر�sessionSocket
	}
	return 0;
}

