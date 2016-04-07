
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

	// проверка параметров
	// имя сервера передаётся через параметр командной строки, если оно не было передано, выводим ошибку
	if (argc != 2) {
		printf("передавайте имя сервера через параметр командной строки: %s server-name\n", argv[0]);
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
		printf("невозможно соединиться с сервером!\n");
		WSACleanup();
		return 1;
	}

	
	// ожидаем после подклбючения команлды от сервера ИГРА НАЧАЛАСЬ

	int receivedCommand = 0;
	iResult = recv(ConnectSocket, (char*)&receivedCommand, sizeof(int), 0);
	if (iResult > 0)
	{
		if (receivedCommand == COMMAND_GAME_STARTED)
			printf("игра началась\n");
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
	// ответим серверу той же командой (мы в игре)
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
	while (true) // в цикле шаги игры
	{
		iResult = recv(ConnectSocket, (char*)&receivedCommand, sizeof(int), 0); // ждём команду Карта Выбрана
		if (iResult > 0)
		{
			if (receivedCommand == COMMAND_CARD_PICKED)
			{
				iResult = recv(ConnectSocket, (char*)&receivedCommand, sizeof(int), 0);
				if (iResult > 0)
				{
					if (receivedCommand == COMMAND_HASH) // ждём команду Хэш идёт
					{
						// и принимаем сам хэш
						iResult = recv(ConnectSocket, (char*)&hash, sizeof(int), 0);
						if (iResult > 0)
						{
							printf("передан хэш: %d\n", hash);
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

				do // запрашиваем догадку о карте (аналогично выбру на сервере)
				{
					printf("сервером выбрана карта из: ");
					for (int i = 0; i < N; i++)
						if (std::find(cardsGone.begin(), cardsGone.end(), i) == cardsGone.end())
						{
							printf("%d", i);
							if (i != N - 1)
								printf(", ");
						}
					printf(".\n");
					printf("ваши предположения? ");
					scanf_s("%d", &guessCard);
				} while (guessCard < 0 || guessCard >= N || std::find(cardsGone.begin(), cardsGone.end(), guessCard) != cardsGone.end());
				
				
				// Отправим сообщение Карта выбрана
				currentCommandCode = COMMAND_CARD_PICKED;
				iSendResult = send(ConnectSocket, (char*)&currentCommandCode, sizeof(int), 0);
				if (iSendResult == SOCKET_ERROR) {
					printf("send failed with error: %d\n", WSAGetLastError());
					closesocket(ConnectSocket);
					WSACleanup();
					return 1;
				}
				// и саму карту
				iSendResult = send(ConnectSocket, (char*)&guessCard, sizeof(int), 0);
				if (iSendResult == SOCKET_ERROR) {
					printf("send failed with error: %d\n", WSAGetLastError());
					closesocket(ConnectSocket);
					WSACleanup();
					return 1;
				}
			}
			else if (receivedCommand == COMMAND_GAME_FINISHED) // а еслди принята команда не Карта выбрана, а Игра закончена, то игра у нас закончена
			{
				printf("игра закончена");
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

		// ждём результат от сервера
		iResult = recv(ConnectSocket, (char*)&receivedCommand, sizeof(int), 0);
		if (iResult > 0)
		{
			if (receivedCommand == COMMAND_CLIENT_RIGHT) // если прав
			{
				printf("вы правы\n");
				// то сообщение выводим, счёт увеличиваем
				clientScore += N - cardsGone.size();
				// запоминаем карту
				cardsGone.push_back(guessCard);
				// и выбрана была именно та, что и клиентом
				pickedCard = guessCard;
			}
			else if (receivedCommand == COMMAND_CLIENT_WRONG) // если не угадал
			{
				printf("вы неправы\n");
				
				// ждём номер загаданной карты
				iResult = recv(ConnectSocket, (char*)&pickedCard, sizeof(int), 0);
				if (iResult > 0)
				{
					cardsGone.push_back(pickedCard);
					printf("%d карта была загадана\n", pickedCard); // выводим её на экрна
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
			printf("счёт клиента: %d\n", clientScore); // выводим счёт
			
			// принимаем  заголовок Случ. Число
			iResult = recv(ConnectSocket, (char*)&receivedCommand, sizeof(int), 0);
			if (iResult > 0)
			{
				if (receivedCommand == COMMAND_RANDOMED)
				{
					// теперь и само число
					iResult = recv(ConnectSocket, (char*)&randomNumber, sizeof(int), 0);
					if (iResult > 0)
					{
						printf("сервер выбрал случайное число: %d\n", randomNumber);
						// вывели его на экран, теперь проверим
						// если оно 0, то карты должна была быть выбрана 0
						// если не 0, то остаток от деления его на карту должен быть 0
						// если это условия не выполнятся, то прислано неверное случайное число
						if (randomNumber == 0 && pickedCard == 0 || randomNumber % pickedCard == 0)
							printf("случайное число в порядке\n");
						else
							printf("случайное число не совпадает с выбранной картой\n");
						// и хэш от этого случайного число должен быть таким же. как и присланный в начале (чтобы не обманывать)
						if (computeHash(randomNumber) == hash)
							printf("хэш в порядке\n");
						else
							printf("хэш неверный\n");
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