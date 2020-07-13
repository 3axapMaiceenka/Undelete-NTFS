#pragma once

#include <windows.h>
#include <memory>
#include <vector> 

namespace ntfs
{
	struct RunlistEntry
	{
		UINT64 m_uRunLength = 0; // in clusters
		INT64  m_lRunOffset = 0; // in clusters from file system beginning

		RunlistEntry() = default;

		RunlistEntry(UINT64 ulRunLenght, INT64 lRunOffset)
			: m_uRunLength(ulRunLenght), m_lRunOffset(lRunOffset)
		{ }
	};

	class Runlist
	{
	public:

		Runlist(CHAR* pRunlistStart, UINT64 uStartingVCN, UINT64 uFinalVCN);

		RunlistEntry& at(WORD wIndex);

		const RunlistEntry& at(WORD wIndex) const;

		WORD size() const;

	private:

		void parseRunlist(CHAR* pRunlistStart);

		template <typename T>
		void zeroHighBytes(T& value, CHAR cBytesNumber)
		{
			T mask = 0;

			while (cBytesNumber != 1)
			{
				mask |= 0xff;
				mask <<= 8;
				cBytesNumber--;
			}

			mask |= 0xff;
			value &= mask;
		}

		void readLength(CHAR** pRunlist, UINT64& uLength, CHAR cBytesNumber);

		void readOffset(CHAR** pRunlist, INT64& lOffset, CHAR cBytesNumber);

		UINT64 m_uStartingVCN;
		UINT64 m_uFinalVCN;
		std::unique_ptr<std::vector<RunlistEntry>> m_pRunlist;
	};

} // namespace

