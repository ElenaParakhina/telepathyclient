
#define WIN32_LEAN_AND_MEAN

#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <stdlib.h>
#include <stdio.h>
#include <vector>
#include <algorithm>
#include <iostream>

#pragma comment (lib, "Ws2_32.lib")
#pragma comment (lib, "Mswsock.lib")
#pragma comment (lib, "AdvApi32.lib")

const int COMMAND_GAME_STARTED = 1;
const int COMMAND_GAME_FINISHED = 2;
const int COMMAND_GAME_ABORTED = 3;
const int COMMAND_CARD_PICKED = 4;
const int COMMAND_HASH = 5;
const int COMMAND_RANDOMED = 6;
const int COMMAND_CLIENT_RIGHT = 7;
const int COMMAND_CLIENT_WRONG = 8;

const int N = 10;
int clientScore = 0;
std::vector<int> cardsGone;

int computeHash(int n)
{
	int result = 0;
	while (n > 0)
	{
		result += n % 10;
		n /= 10;
	}

	return result;
}

#define DEFAULT_BUFLEN 512
#define DEFAULT_PORT "27015"

int __cdecl main(int argc, char **argv)
{
	setlocale(LC_ALL, "rus");
	WSADATA wsaData;
	SOCKET ConnectSocket = INVALID_SOCKET;
	struct addrinfo *result = NULL,
		*ptr = NULL,
		hints;
	char *sendbuf = "this is a test";
	char recvbuf[DEFAULT_BUFLEN];
	int iResult;
	int recvbuflen = DEFAULT_BUFLEN;

	// �������� ����������
	// ��� ������� ��������� ����� �������� ��������� ������, ���� ��� �� ���� ��������, ������� ������
	if (argc != 2) {
		printf("����������� ��� ������� ����� �������� ��������� ������: %s server-name\n", argv[0]);
		return 1;
	}

	// Initialize Winsock
	iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
	if (iResult != 0) {
		printf("WSAStartup failed with error: %d\n", iResult);
		return 1;
	}

	ZeroMemory(&hints, sizeof(hints));
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_protocol = IPPROTO_TCP;

	// Resolve the server address and port
	iResult = getaddrinfo(argv[1], DEFAULT_PORT, &hints, &result);
	if (iResult != 0) {
		printf("getaddrinfo failed with error: %d\n", iResult);
		WSACleanup();
		return 1;
	}

	// Attempt to connect to an address until one succeeds
	for (ptr = result; ptr != NULL; ptr = ptr->ai_next) {

		// Create a SOCKET for connecting to server
		ConnectSocket = socket(ptr->ai_family, ptr->ai_socktype,
			ptr->ai_protocol);
		if (ConnectSocket == INVALID_SOCKET) {
			printf("socket failed with error: %ld\n", WSAGetLastError());
			WSACleanup();
			return 1;
		}

		// Connect to server.
		iResult = connect(ConnectSocket, ptr->ai_addr, (int)ptr->ai_addrlen);
		if (iResult == SOCKET_ERROR) {
			closesocket(ConnectSocket);
			ConnectSocket = INVALID_SOCKET;
			continue;
		}
		break;
	}

	freeaddrinfo(result);

	if (ConnectSocket == INVALID_SOCKET) {
		printf("���������� ����������� � ��������!\n");
		WSACleanup();
		return 1;
	}

	
	// ������� ����� ������������ �������� �� ������� ���� ��������

	int receivedCommand = 0;
	iResult = recv(ConnectSocket, (char*)&receivedCommand, sizeof(int), 0);
	if (iResult > 0)
	{
		if (receivedCommand == COMMAND_GAME_STARTED)
			printf("���� ��������\n");
	}
	else if (iResult == 0)
	{
		printf("Connection closed");
		return 0;
	}
	else
	{
		printf("recv failed with error: %d\n", WSAGetLastError());
		closesocket(ConnectSocket);
		WSACleanup();
		return 1;
	}

	int iSendResult;
	// ������� ������� ��� �� �������� (�� � ����)
	int currentCommandCode = COMMAND_GAME_STARTED;
	iSendResult = send(ConnectSocket, (char*)&currentCommandCode, sizeof(int), 0);
	if (iSendResult == SOCKET_ERROR) {
		printf("send failed with error: %d\n", WSAGetLastError());
		closesocket(ConnectSocket);
		WSACleanup();
		return 1;
	}

	int pickedCard;
	int guessCard;
	int hash;
	int randomNumber;
	while (true) // � ����� ���� ����
	{
		iResult = recv(ConnectSocket, (char*)&receivedCommand, sizeof(int), 0); // ��� ������� ����� �������
		if (iResult > 0)
		{
			if (receivedCommand == COMMAND_CARD_PICKED)
			{
				iResult = recv(ConnectSocket, (char*)&receivedCommand, sizeof(int), 0);
				if (iResult > 0)
				{
					if (receivedCommand == COMMAND_HASH) // ��� ������� ��� ���
					{
						// � ��������� ��� ���
						iResult = recv(ConnectSocket, (char*)&hash, sizeof(int), 0);
						if (iResult > 0)
						{
							printf("������� ���: %d\n", hash);
						}
						else if (iResult == 0)
						{
							printf("Connection closed");
							return 0;
						}
						else
						{
							printf("recv failed with error: %d\n", WSAGetLastError());
							closesocket(ConnectSocket);
							WSACleanup();
							return 1;
						}
					}
				}
				else if (iResult == 0)
				{
					printf("Connection closed");
					return 0;
				}
				else
				{
					printf("recv failed with error: %d\n", WSAGetLastError());
					closesocket(ConnectSocket);
					WSACleanup();
					return 1;
				}

				do // ����������� ������� � ����� (���������� ����� �� �������)
				{
					printf("�������� ������� ����� ��: ");
					for (int i = 0; i < N; i++)
						if (std::find(cardsGone.begin(), cardsGone.end(), i) == cardsGone.end())
						{
							printf("%d", i);
							if (i != N - 1)
								printf(", ");
						}
					printf(".\n");
					printf("���� �������������? ");
					scanf_s("%d", &guessCard);
				} while (guessCard < 0 || guessCard >= N || std::find(cardsGone.begin(), cardsGone.end(), guessCard) != cardsGone.end());
				
				
				// �������� ��������� ����� �������
				currentCommandCode = COMMAND_CARD_PICKED;
				iSendResult = send(ConnectSocket, (char*)&currentCommandCode, sizeof(int), 0);
				if (iSendResult == SOCKET_ERROR) {
					printf("send failed with error: %d\n", WSAGetLastError());
					closesocket(ConnectSocket);
					WSACleanup();
					return 1;
				}
				// � ���� �����
				iSendResult = send(ConnectSocket, (char*)&guessCard, sizeof(int), 0);
				if (iSendResult == SOCKET_ERROR) {
					printf("send failed with error: %d\n", WSAGetLastError());
					closesocket(ConnectSocket);
					WSACleanup();
					return 1;
				}
			}
			else if (receivedCommand == COMMAND_GAME_FINISHED) // � ����� ������� ������� �� ����� �������, � ���� ���������, �� ���� � ��� ���������
			{
				printf("���� ���������");
			}
		}
		else if (iResult == 0)
		{
			printf("Connection closed");
			return 0;
		}
		else
		{
			printf("recv failed with error: %d\n", WSAGetLastError());
			closesocket(ConnectSocket);
			WSACleanup();
			return 1;
		}

		// ��� ��������� �� �������
		iResult = recv(ConnectSocket, (char*)&receivedCommand, sizeof(int), 0);
		if (iResult > 0)
		{
			if (receivedCommand == COMMAND_CLIENT_RIGHT) // ���� ����
			{
				printf("�� �����\n");
				// �� ��������� �������, ���� �����������
				clientScore += N - cardsGone.size();
				// ���������� �����
				cardsGone.push_back(guessCard);
				// � ������� ���� ������ ��, ��� � ��������
				pickedCard = guessCard;
			}
			else if (receivedCommand == COMMAND_CLIENT_WRONG) // ���� �� ������
			{
				printf("�� �������\n");
				
				// ��� ����� ���������� �����
				iResult = recv(ConnectSocket, (char*)&pickedCard, sizeof(int), 0);
				if (iResult > 0)
				{
					cardsGone.push_back(pickedCard);
					printf("%d ����� ���� ��������\n", pickedCard); // ������� � �� �����
				}
				else if (iResult == 0)
				{
					printf("Connection closed");
					return 0;
				}
				else
				{
					printf("recv failed with error: %d\n", WSAGetLastError());
					closesocket(ConnectSocket);
					WSACleanup();
					return 1;
				}
			}
			printf("���� �������: %d\n", clientScore); // ������� ����
			
			// ���������  ��������� ����. �����
			iResult = recv(ConnectSocket, (char*)&receivedCommand, sizeof(int), 0);
			if (iResult > 0)
			{
				if (receivedCommand == COMMAND_RANDOMED)
				{
					// ������ � ���� �����
					iResult = recv(ConnectSocket, (char*)&randomNumber, sizeof(int), 0);
					if (iResult > 0)
					{
						printf("������ ������ ��������� �����: %d\n", randomNumber);
						// ������ ��� �� �����, ������ ��������
						// ���� ��� 0, �� ����� ������ ���� ���� ������� 0
						// ���� �� 0, �� ������� �� ������� ��� �� ����� ������ ���� 0
						// ���� ��� ������� �� ����������, �� �������� �������� ��������� �����
						if (randomNumber == 0 && pickedCard == 0 || randomNumber % pickedCard == 0)
							printf("��������� ����� � �������\n");
						else
							printf("��������� ����� �� ��������� � ��������� ������\n");
						// � ��� �� ����� ���������� ����� ������ ���� ����� ��. ��� � ���������� � ������ (����� �� ����������)
						if (computeHash(randomNumber) == hash)
							printf("��� � �������\n");
						else
							printf("��� ��������\n");
					}
					else if (iResult == 0)
					{
						printf("Connection closed");
						return 0;
					}
					else
					{
						printf("recv failed with error: %d\n", WSAGetLastError());
						closesocket(ConnectSocket);
						WSACleanup();
						return 1;
					}
				}
			}
			else if (iResult == 0)
			{
				printf("Connection closed");
				return 0;
			}
			else
			{
				printf("recv failed with error: %d\n", WSAGetLastError());
				closesocket(ConnectSocket);
				WSACleanup();
				return 1;
			}
		}
		else if (iResult == 0)
		{
			printf("Connection closed");
			return 0;
		}
		else
		{
			printf("recv failed with error: %d\n", WSAGetLastError());
			closesocket(ConnectSocket);
			WSACleanup();
			return 1;
		}

	}

	// cleanup
	closesocket(ConnectSocket);
	WSACleanup();

	return 0;
}