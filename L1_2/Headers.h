#ifndef HEADERS_H
#define HEADERS_H

struct IP_HEADER {
	BYTE		ihl : 4;           // Length of the header in dwords
	BYTE		version : 4;         // Version of IP
	BYTE		tos;               // Type of service
	WORD		tl;       // Length of the packet in dwords
	WORD		id;           // unique identifier
	WORD		flags : 3;           // Flags
	WORD		fragmentOffset : 13;
	BYTE		ttl;               // Time to live
	BYTE		proto;             // Protocol number (TCP, UDP etc)
	WORD		checksum;        // IP checksum
	DWORD		sourceIp;
	DWORD		destIp;
};

// ICMP header
struct ICMP_HEADER {
	BYTE		type;          // ICMP packet type
	BYTE		code;          // Type sub code
	WORD		checksum;
	WORD		id;
	WORD		sequence;
	DWORD		timestamp;    // not part of ICMP, but we need it
};

typedef struct Data {
	char* host;
	bool isPing;
} MYDATA, *PMYDATA;
#endif //HEADERS_H
