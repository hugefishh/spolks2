#ifndef ICMP_H
#define ICMP_H

#include <WinSock2.h>
#include <WS2tcpip.h>
#include <Windows.h>

#include "Headers.h"


#define ICMP_ECHO_REPLY		0
#define ICMP_ECHO_REQUEST	8
#define ICMP_TIME_EXCEEDED  11
#define IP_LEN				16
#define ICMP_DATA_SIZE		32
#define RECV_DATA_SIZE		1024
#define MAX_HOP				30
#define MAX_RETRIES			3
#define PING_HOPS			5
#define MAX_THREADS			10
#define BUFFER_SIZE			150

bool SettingSocket(const char* ip, SOCKET& s, sockaddr_in& dest);

bool SetTTL(const SOCKET& s, const int& ttl);

DWORD WINAPI Tracert(LPVOID lpParam);

bool GetIP(const char* host, char* ip, const size_t ipSize);

//void outputTracert(char*, long*);

void output(char*);
#endif //ICMP_H
