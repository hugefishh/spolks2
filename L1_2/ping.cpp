#include "ICMP.h"
#include <iostream>
#include <string.h>
#include <memory>
#include <assert.h>
#include "checksum.h"

bool ping(const char * host)
{
	assert(host && "invalid host\n");

	using std::cerr;
	using std::cout;
	using std::unique_ptr;
	using std::make_unique;
	using std::endl;

	char ip[IP_LEN] = { 0 };
	if (!GetIP(host, ip, IP_LEN))
	{
		//cerr << "GetIP() " << WSAGetLastError() << endl;
		return false;
	}

	SOCKET s;
	sockaddr_in destAddr;
	if (!SettingSocket(ip, s, destAddr))
	{
		//cerr << "SettingSocket() " << WSAGetLastError() << endl;
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
	for (int nHop = 0; nHop < PING_HOPS; nHop++)
	{
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
				return false;
			}
			fd_set fdRead;
			timeval selectTime = { 3,0 };

			FD_ZERO(&fdRead);
			FD_SET(s, &fdRead);

			result = select(0, &fdRead, nullptr, nullptr, &selectTime);
			if (result == SOCKET_ERROR)
			{
				closesocket(s);
				cerr << "\nselect() " << WSAGetLastError() << endl;
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
					return false;
				}

				recvIP = (IP_HEADER*)recvBuffer.get();
				recvICMP = (ICMP_HEADER*)(recvBuffer.get() + (recvIP->ihl << 2));
				recvICMP->timestamp = ntohl(recvICMP->timestamp);


				cout << "\t" << recvTickCount - sendTickCount << " ms";
			}
			else
			{
				cout << "\t *";
			}
			nRetries++;
		}

		if (bResponse)
		{
			in_addr ipAdd = recvAddr.sin_addr;

			char sourceIp[IP_LEN] = { 0 };
			inet_ntop(AF_INET, &ipAdd, sourceIp, IP_LEN);

			cout << "\t" << sourceIp << endl;

			if (recvICMP->type == ICMP_ECHO_REPLY)
			{
				break;
			}
		}
		else
		{
			cout << "\tRequest timed out\n";
		}

		Sleep(1000);
	}

	return true;
}
