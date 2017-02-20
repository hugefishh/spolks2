#include "ICMP.h"
#include <iostream>
#include <string.h>
#include <memory>
#include <assert.h>
#include "checksum.h"
#include <mutex>

std::mutex m;

bool SettingSocket(const char* ip, SOCKET & s, sockaddr_in & dest)
{
	s = socket(AF_INET, SOCK_RAW, IPPROTO_ICMP);
	if (s == SOCKET_ERROR)
	{
		std::cout << "Socket not opened for ICMP" <<  WSAGetLastError() << std::endl;
		return false;
	}

	const size_t size = 60 * 1024;
	if (setsockopt(s, SOL_SOCKET, SO_RCVBUF, (const char*)&size, sizeof(size)) == SOCKET_ERROR)
	{
		std::cout << "Socket opts not set" << std::endl;
		return false;
	}

	timeval timeout;
	timeout.tv_sec = 3;
	timeout.tv_usec = 0;
	if (setsockopt(s, SOL_SOCKET, SO_RCVTIMEO, (const char*)&timeout, sizeof(struct timeval)) == SOCKET_ERROR)
	{
		std::cout << "Socket opts 2 not set" << std::endl;
		return false;
	}

	addrinfo* result = nullptr;
	if (getaddrinfo(ip, nullptr, nullptr, &result) != 0)
	{
		std::cout << "Address not found" << std::endl;
		return false;
	}
	ZeroMemory(&dest, sizeof(dest));

	dest.sin_addr = ((sockaddr_in*)result->ai_addr)->sin_addr;
	dest.sin_family = AF_INET;
	freeaddrinfo(result);

	return true;
}

bool SetTTL(const SOCKET& s, const int& ttl)
{
	return SOCKET_ERROR != setsockopt(s, IPPROTO_IP, IP_TTL, (const char*)&ttl, sizeof(ttl));
}

void output(char* data) {
	using std::cout;
	using std::endl;

	cout << data<<endl;
}

void outputTracert(char** resultHosts, long* timings)
{
	using std::cout;
	using std::endl;

	for (int nHop = 0; nHop < MAX_HOP; nHop++)
	{
		if (resultHosts[nHop][0] < '0' || resultHosts[nHop][0]>'2') break;
		cout << " " << nHop + 1;
		for (int nRetries = 0; nRetries < MAX_RETRIES; nRetries++) {
			cout << "\t" << timings[nHop + nRetries] << " ms";
		}
		cout << "\t" << resultHosts[nHop] << endl;
	}
}

void outputPing(char* host, long time, long TTL) {
	using std::cout;
	using std::endl;

	cout << "Reply from " << host << ": time = " << time << " TTL = " << TTL << endl;
}

DWORD WINAPI Tracert(LPVOID params)
{
	std::cout << "ICMP sening thread started." << std::endl;
	PMYDATA data;
	data = (PMYDATA)params;
	char* host = data->host;
	bool isPing = data->isPing;
	assert(host && "invalid host\n");

	using std::cerr;
	using std::unique_ptr;
	using std::make_unique;
	using std::endl;

	char** resultHosts = new char*[MAX_HOP];
	for (int i = 0; i < MAX_HOP; ++i)
		resultHosts[i] = new char[MAX_HOP * 3];
	long timings[MAX_HOP * 3];

	char ip[IP_LEN] = { 0 };	
	if (!GetIP(host, ip, IP_LEN))
	{
		//cerr << "GetIP() " << WSAGetLastError() << endl;
		std::cout << "Wrong host. Closing thread" << std::endl;
		return false;
	}
	std::cout << "Host is correct" << std::endl;

	char message[BUFFER_SIZE];
	memset(message, 0, sizeof(message));
	//sprintf_s(message, "Tracing route to %s [%s]\n with the maximum number of hops: %d\n", host, ip, MAX_HOP);
	std::cout << "Tracing route to " << host << "[" << ip << "]\n with the maximum number of hops: "<< MAX_HOP <<"\n" << std::endl;

	SOCKET s;
	sockaddr_in destAddr;
	if (!SettingSocket(ip, s, destAddr))
	{
		//cerr << "SettingSocket() " << WSAGetLastError() << endl;
		std::cout << "Socket not opened" << endl;
		return false;
	}

	const size_t icmpPacketSize = sizeof(ICMP_HEADER);// +ICMP_DATA_SIZE;
	unique_ptr<char[]> sendBuffer = make_unique<char[]>(icmpPacketSize);
	unique_ptr<char[]> recvBuffer = make_unique<char[]>(RECV_DATA_SIZE);

	ICMP_HEADER* icmpH = (ICMP_HEADER*)sendBuffer.get();
	icmpH->type = ICMP_ECHO_REQUEST;
	icmpH->id = GetCurrentProcessId();
		
	int ttl = 1;
	int nSequence = 1;
	for (int nHop = 0; nHop < MAX_HOP; nHop++)
	{
		std::cout << "Sening package" << endl;
		if (!isPing)
		{
			SetTTL(s, ttl++);
		}

		IP_HEADER* recvIP;
		ICMP_HEADER* recvICMP;

		bool bResponse = false;
		uint8_t nRetries = 0;

		sockaddr_in recvAddr = { 0 };
		int sizeRecvAddr;
		sizeRecvAddr = sizeof(recvAddr);
		while (nRetries < MAX_RETRIES)
		{
			icmpH->checksum = 0;
			icmpH->sequence = nSequence++;
			icmpH->checksum = Checksum((WORD*)sendBuffer.get(), icmpPacketSize);

			int result = sendto(s, sendBuffer.get(), icmpPacketSize, 0, (sockaddr*)&destAddr, sizeof(destAddr));
			const unsigned long sendTickCount = GetTickCount();
			if (result == SOCKET_ERROR)
			{
				closesocket(s);
				
				//cerr << "\nsendto() " << WSAGetLastError() << endl;
				std::cout << "Socket error" << endl;
				return false;
			}
			fd_set fdRead;
			timeval selectTime = { 6,0 };

			FD_ZERO(&fdRead);
			FD_SET(s, &fdRead);

			result = select(0, &fdRead, nullptr, nullptr, &selectTime);
			std::cout << "Result got" << endl;
			if (result == SOCKET_ERROR)
			{
				closesocket(s);
				cerr << "\nselect() " << WSAGetLastError() << endl;
				std::cout << "\nselect() " << WSAGetLastError() << endl;
				return false;
			}

			if (result > 0 && FD_ISSET(s, &fdRead))
			{
				bResponse = true;

				result = recvfrom(s, recvBuffer.get(), RECV_DATA_SIZE, 0, (sockaddr*)&recvAddr, &sizeRecvAddr);
				const unsigned long recvTickCount = GetTickCount();
				
				if (result == SOCKET_ERROR)
				{
					closesocket(s);

					cerr << "\nrecvfrom() " << WSAGetLastError() << endl;
					std::cout << "\nrecvfrom() " << WSAGetLastError() << endl;
					return false;
				}

				recvIP = (IP_HEADER*)recvBuffer.get();
				recvICMP = (ICMP_HEADER*)(recvBuffer.get() + (recvIP->ihl << 2));
				recvICMP->timestamp = ntohl(recvICMP->timestamp);
				if (recvICMP->type == ICMP_TIME_EXCEEDED)
				{
					timings[nHop + nRetries] = -1;
				}
				long time = recvTickCount - sendTickCount;
				timings[nHop + nRetries] = time;
				std::cout << "\nrecvfrom() " << WSAGetLastError() << endl;
				if (isPing) {
					m.lock();
					outputPing(host, time, ttl);
					m.unlock();
				}
			}
			nRetries++;
		}

		if (bResponse)
		{
			std::cout << "\nrecvfrom() " << WSAGetLastError() << endl;
			in_addr ipAdd = recvAddr.sin_addr;
			
			char sourceIp[IP_LEN] = { 0 };
			inet_ntop(AF_INET, &ipAdd, sourceIp, IP_LEN);
			strcpy(resultHosts[nHop], sourceIp);



			if (recvICMP->type == ICMP_ECHO_REPLY)
			{
				break;
			}
		}
		else
		{
			strcpy(resultHosts[nHop], "Request timed out\n");
		}	
		
	}
	std::cout << "TEST" << endl;
	if(!isPing){
		m.lock();
		output(message);
		outputTracert(resultHosts, timings);
		m.unlock();
	}
	return true;
}


bool GetIP(const char * host, char * ip, const size_t ipSize)
{
	sockaddr_in sockAddrIn;
	const size_t len = min(strlen(host), ipSize);
	if (inet_pton(AF_INET, host, &sockAddrIn) == 1)
	{	
		memcpy_s(ip, len, host, len);
	}
	else
	{
		addrinfo* result = nullptr;
		if (getaddrinfo(host, nullptr, nullptr, &result) != 0)
		{
			return false;
		}
		ZeroMemory(&sockAddrIn, sizeof(sockAddrIn));

		sockAddrIn.sin_addr = ((sockaddr_in*)result->ai_addr)->sin_addr;
		sockAddrIn.sin_family = AF_INET;
		freeaddrinfo(result);

		if (!inet_ntop(AF_INET, &sockAddrIn.sin_addr, ip, ipSize))
		{
			return false;
		}
	}

	return true;
}