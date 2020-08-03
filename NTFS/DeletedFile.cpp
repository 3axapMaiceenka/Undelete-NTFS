#include "DeletedFile.h"
#include "NTFSDataStructures.h"

#include <iostream>

ntfs::DeletedFile::DeletedFile()
	: m_uRecordAddress(0),
	m_pszFileName(NULL),
	m_pFileNameAttr(NULL),
	m_cType(0)
{ }

ntfs::DeletedFile::DeletedFile(UINT64 uRecordAddress, const FILE_NAME_ATTR& fileNameAttr, const WCHAR* pszFileName, CHAR cType)
	: m_uRecordAddress(uRecordAddress),
	m_pszFileName(new WCHAR[wcslen(pszFileName) + 1]),
	m_pFileNameAttr(new FILE_NAME_ATTR(fileNameAttr)),
	m_cType(cType)
{
	auto length = wcslen(pszFileName);
	wmemcpy_s(m_pszFileName, length + 1, pszFileName, length);
	m_pszFileName[length] = L'\0';
}

ntfs::DeletedFile::DeletedFile(const DeletedFile& rhs)
	: DeletedFile(rhs.m_uRecordAddress, *rhs.m_pFileNameAttr, rhs.m_pszFileName, rhs.m_cType)
{ }

ntfs::DeletedFile::DeletedFile(DeletedFile&& rhs) noexcept
	: m_uRecordAddress(rhs.m_uRecordAddress),
	m_pszFileName(rhs.m_pszFileName),
	m_pFileNameAttr(rhs.m_pFileNameAttr),
	m_cType(rhs.m_cType)
{
	rhs.m_pFileNameAttr = NULL;
	rhs.m_pszFileName = NULL;
	rhs.m_cType = rhs.m_uRecordAddress = 0;
}

void ntfs::DeletedFile::free()
{
	delete m_pFileNameAttr;
	delete m_pszFileName;
}

ntfs::DeletedFile& ntfs::DeletedFile::operator=(const DeletedFile& rhs)
{
	if (this != &rhs)
	{
		free();

		m_uRecordAddress = rhs.m_uRecordAddress;
		m_pFileNameAttr = new FILE_NAME_ATTR(*rhs.m_pFileNameAttr);
		m_cType = rhs.m_cType;

		m_pszFileName = new WCHAR[wcslen(rhs.m_pszFileName) + 1];
		wmemcpy_s(m_pszFileName, wcslen(rhs.m_pszFileName) + 1, rhs.m_pszFileName, wcslen(rhs.m_pszFileName));
	}

	return *this;
}

ntfs::DeletedFile& ntfs::DeletedFile::operator=(DeletedFile&& rhs) noexcept
{
	if (this != &rhs)
	{
		free();

		m_uRecordAddress = rhs.m_uRecordAddress;
		m_pFileNameAttr = rhs.m_pFileNameAttr;
		m_pszFileName = rhs.m_pszFileName;
		m_cType = rhs.m_cType;

		rhs.m_cType = rhs.m_uRecordAddress = 0;
		rhs.m_pszFileName = NULL;
		rhs.m_pFileNameAttr = NULL;
	}

	return *this;
}

bool ntfs::DeletedFile::operator==(const DeletedFile& rhs) const
{
	if (m_pszFileName && rhs.m_pszFileName)
	{
		if (wcscmp(m_pszFileName, rhs.m_pszFileName)) return false;
	}
	else
	{
		if (m_pszFileName != rhs.m_pszFileName) return false;
	}

	if (m_pFileNameAttr && rhs.m_pFileNameAttr)
	{
		return m_pFileNameAttr->m_uParentCatalogBaseAddress == rhs.m_pFileNameAttr->m_uParentCatalogBaseAddress &&
			   m_pFileNameAttr->m_uModificationTime == rhs.m_pFileNameAttr->m_uModificationTime &&
			   m_pFileNameAttr->m_uMFTModificationTime == rhs.m_pFileNameAttr->m_uMFTModificationTime &&
			   m_pFileNameAttr->m_uFileCreationTime == rhs.m_pFileNameAttr->m_uFileCreationTime &&
			   m_pFileNameAttr->m_uFileAccessTime == rhs.m_pFileNameAttr->m_uFileAccessTime &&
			   m_cType == rhs.m_cType;
	}
	else
	{
		return m_pFileNameAttr == rhs.m_pFileNameAttr && m_cType == rhs.m_cType;
	}
}

bool ntfs::DeletedFile::operator!=(const DeletedFile& rhs) const
{
	return !(*this == rhs);
}

FILETIME ntfs::constructFileTime(UINT64 uValue)
{
	FILETIME fileTime;
	fileTime.dwHighDateTime = uValue >> 32;
	fileTime.dwLowDateTime = uValue & 0x00000000ffffffff;

	return fileTime;
}

std::ostream& ntfs::printDate(std::ostream& os, const char* message, const SYSTEMTIME& systemTime)
{
	os << message;
	systemTime.wDay > 9 ? os << systemTime.wDay : os << "0" << systemTime.wDay; os << ".";
	systemTime.wMonth > 9 ? os << systemTime.wMonth : os << "0" << systemTime.wMonth; os << ".";
	os << systemTime.wYear << " ";
	systemTime.wHour > 9 ? os << systemTime.wHour : os << "0" << systemTime.wHour; os << ".";
	systemTime.wMinute > 9 ? os << systemTime.wMinute : os << "0" << systemTime.wMinute; os << ".";
	systemTime.wSecond > 9 ? os << systemTime.wSecond : os << "0" << systemTime.wSecond;

	return os;
}

std::ostream& operator<<(std::ostream& os, const ntfs::DeletedFile& df)
{
	if (df.m_pszFileName)
	{
		os << CString(df.m_pszFileName) << std::endl;
	}

	if (df.m_pFileNameAttr)
	{
		SYSTEMTIME sysTime;
		FILETIME fileTime = ntfs::constructFileTime(df.m_pFileNameAttr->m_uFileCreationTime);
		FileTimeToSystemTime(&fileTime, &sysTime);
		ntfs::printDate(os, "\tFile creation time: ", sysTime) << std::endl;

		fileTime = ntfs::constructFileTime(df.m_pFileNameAttr->m_uFileAccessTime);
		FileTimeToSystemTime(&fileTime, &sysTime);
		ntfs::printDate(os, "\tFile access time: ", sysTime) << std::endl;

		fileTime = ntfs::constructFileTime(df.m_pFileNameAttr->m_uMFTModificationTime);
		FileTimeToSystemTime(&fileTime, &sysTime);
		ntfs::printDate(os, "\tFile modification time: ", sysTime) << std::endl;

		os << "\tFile size: " << df.m_pFileNameAttr->m_uActualFileSize << std::endl;
		os << "\tFile type: ";
		df.m_cType == ntfs::DELETED_FILE ? os << "file" << std::endl : os << "directory" << std::endl;
	}

	return os;
} 

ntfs::DeletedFile::~DeletedFile()
{
	free();
}

