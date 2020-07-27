#pragma once

#include <windows.h>
#include <list>
#include <memory>

namespace ntfs
{

#define BUF_SIZE 1024

	struct MFTInfo;
	struct DeletedFile;
	struct MFTEntryHeader;
	struct VolumeInfo;
	struct AttributeHeader;
	class  Runlist;

	class MFTParser
	{
	public:

		MFTParser(const MFTInfo& mftInfo);

		~MFTParser();

		const VolumeInfo getVolumeInfo() const;

		void findDeletedFiles();

		const std::shared_ptr<std::list<DeletedFile>> getDeletedFiles() const;

		BOOLEAN undelete(const DeletedFile* const pDeletedFile, LPCSTR lpszDirectoryName);

	private:
		using PointerToMemberFunction = BOOLEAN(MFTParser::*)(MFTEntryHeader*, UINT64);

		BOOLEAN checkForDeleted(MFTEntryHeader* pHeader, UINT64 uRecordAddress);

		BOOLEAN findVolumeAttributes(MFTEntryHeader* pHeader, UINT64 uAddress);

		BOOLEAN findBitmap(MFTEntryHeader* pHeader, UINT64 uAddress);

		BOOLEAN seek(UINT64 uDIstance, DWORD dwMoveMethod) const;

		void readFirstRecord();

		WCHAR* readUtf16String(CHAR* pSource, WORD wLength) const;

		void iterate(PointerToMemberFunction jobFunc);

		const AttributeHeader* findAttr(const AttributeHeader* pAttrHeader, UINT32 uAttrType) const;

		BOOLEAN isDataCleared(const AttributeHeader* pAttrHeader) const;

		BOOLEAN checkIfAllocated(UINT64 uNumberOfClusters, UINT64 uOffsetInBytes, UINT64 uOffsetInBits) const;

		BOOLEAN intersect(const Runlist* const pLhs, const Runlist* const pRhs, WORD wLhsRunIndx, WORD wRhsRunIndx) const;

		void undeleteFile(HANDLE hFile, const AttributeHeader* pAttrHeader) const;

		UINT64   m_uBitmapOffset;
		MFTInfo* m_pMft;
		Runlist* m_pRunlist;
		Runlist* m_pBitmapRunlist;
		HANDLE   m_hDrive;
		VolumeInfo* m_pVolumeInfo;
		std::shared_ptr<std::list<DeletedFile>> m_pDeletedFiles;
	};

} // namespace