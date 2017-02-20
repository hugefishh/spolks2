#ifndef CHECKSUM_H
#define CHECKSUM_H

WORD Checksum(WORD* data, WORD length) 
{
	DWORD dwCkSum = 0;
	while (length > 1)
	{
		dwCkSum += *data++;
		length -= 2;
	}

	if (length > 1)
	{
		dwCkSum += *(BYTE*)data;
	}

	dwCkSum = (dwCkSum >> 16) + (dwCkSum & 0xFFFF);
	dwCkSum += (dwCkSum >> 16);

	return ~(WORD)dwCkSum;
}

#endif //CHECKSUM_H
