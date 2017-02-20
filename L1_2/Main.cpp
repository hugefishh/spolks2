#include <assert.h>
#include "ICMP.h"

#pragma comment(lib, "Ws2_32.lib")


int main(int argc, char** argv)
{
	if(argc <2)
	{
		return 1;
	}
	WSADATA wsaData;
	PMYDATA data[MAX_THREADS];
	HANDLE  hThreadArray[MAX_THREADS];

	const int err = WSAStartup(MAKEWORD(2, 2), &wsaData);
	assert(SOCKET_ERROR != err && "WSAStartup() failed.\n");
	bool isPing = false;
	if (!strcmp(argv[1], "ping"))
		isPing = true;
	for (int i = 0; i < argc - 2; i++) {

		data[i] = (PMYDATA)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY,
			sizeof(MYDATA));

		if (data[i] == NULL)
		{
			ExitProcess(2);
		}

		data[i]->host = argv[i+2];
		data[i]->isPing = isPing;

		hThreadArray[i] = CreateThread(NULL, 0, Tracert, data[i], 0, NULL);

		if (hThreadArray[i] == NULL)
		{
			ExitProcess(3);
		}
	}

	WaitForMultipleObjects(argc - 2, hThreadArray, TRUE, INFINITE);


	for (int i = 0; i<argc - 2; i++)
	{
		CloseHandle(hThreadArray[i]);
		if (data[i] != NULL)
		{
			HeapFree(GetProcessHeap(), 0, data[i]);
			data[i] = NULL;
		}
	}
	system("pause");
	return 0;
}
