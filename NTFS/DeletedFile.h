#pragma once

#include <windows.h>

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

		~DeletedFile();

		UINT64 m_uRecordAddress;
		WCHAR* m_pszFileName;
		FILE_NAME_ATTR* m_pFileNameAttr;
		CHAR m_cType; // ntfs::MFT_ENTRY_HEADER_FLAGS.DELETED_FILE or ntfs::MFT_ENTRY_HEADER_FLAGS.DELETED_CATALOG
	};

}
