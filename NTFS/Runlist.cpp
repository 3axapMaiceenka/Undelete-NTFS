#include "Runlist.h"

ntfs::Runlist::Runlist(CHAR* pcRunlistStart, UINT64 uStartingVCN, UINT64 uFinalVCN)
	: m_uStartingVCN(uStartingVCN),
	m_uFinalVCN(uFinalVCN),
	m_pRunlist(std::make_unique<std::vector<RunlistEntry>>())
{
	parseRunlist(pcRunlistStart);
}

const ntfs::RunlistEntry& ntfs::Runlist::at(WORD wIndex) const
{
	return m_pRunlist->at(wIndex);
}

ntfs::RunlistEntry& ntfs::Runlist::at(WORD wIndex)
{
	return m_pRunlist->at(wIndex);
}

WORD ntfs::Runlist::size() const
{
	return m_pRunlist->size();
}

void ntfs::Runlist::parseRunlist(CHAR* pcRunlistStart)
{
	CHAR cHeader;
	INT64 lOffset = 0, lPrevOffset = 0;
	UINT64 uLength = 0;

	while ((cHeader = *pcRunlistStart++) != 0)
	{
		CHAR cOffsetFieldSize = (cHeader & 0xf0) >> 4;
		CHAR cLengthFieldSize = cHeader & 0x0f;

		readLength(&pcRunlistStart, uLength, cLengthFieldSize);
		readOffset(&pcRunlistStart, lOffset, cOffsetFieldSize);

		lOffset += lPrevOffset;
		lPrevOffset = lOffset;

		m_pRunlist->emplace_back(uLength, lOffset);
	}
}

void ntfs::Runlist::readLength(CHAR** pRunlist, UINT64& uLength, CHAR cBytesNumber)
{
	uLength = **((UINT64**)pRunlist);

	zeroHighBytes<UINT64>(uLength, cBytesNumber);

	*pRunlist += cBytesNumber;
}

void ntfs::Runlist::readOffset(CHAR** pRunlist, INT64& lOffset, CHAR cBytesNumber)
{
	lOffset = **((INT64**)pRunlist);

	zeroHighBytes<INT64>(lOffset, cBytesNumber);

	if (lOffset >> (cBytesNumber * 8 - 1) && cBytesNumber != 8) // if negative value
	{
		INT64 lMask = (INT64)0xff00000000000000 >> ((8 - cBytesNumber - 1) * 8); // set high bits to 1
		lOffset |= lMask;
	}

	*pRunlist += cBytesNumber;
}
