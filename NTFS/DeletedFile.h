#pragma once

#include <windows.h>
#include <iostream>

namespace ntfs
{
	struct FILE_NAME_ATTR;

	struct DeletedFile
	{
	private:

		void free();

	public:

		DeletedFile();

		DeletedFile(UINT64 uRecordAddress, const FILE_NAME_ATTR& fileNameAttr, const WCHAR* pszFileName, CHAR cType);

		DeletedFile(const DeletedFile& rhs);

		DeletedFile(DeletedFile&& rhs) noexcept;

		DeletedFile& operator=(const DeletedFile& rhs);

		DeletedFile& operator=(DeletedFile&& rhs) noexcept;

		bool operator==(const DeletedFile& rhs) const;

		bool operator!=(const DeletedFile& rhs) const;

		~DeletedFile();

		UINT64 m_uRecordAddress;
		WCHAR* m_pszFileName;
		FILE_NAME_ATTR* m_pFileNameAttr;
		CHAR m_cType; // ntfs::MFT_ENTRY_HEADER_FLAGS.DELETED_FILE or ntfs::MFT_ENTRY_HEADER_FLAGS.DELETED_CATALOG
	};

	std::ostream& printDate(std::ostream& os, const char* message, const SYSTEMTIME& systemTime);

	FILETIME constructFileTime(UINT64 uValue);

} // namespace

std::ostream& operator<<(std::ostream& os, const ntfs::DeletedFile& df);
