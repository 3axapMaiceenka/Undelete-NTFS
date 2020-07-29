#include "DeletedFile.h"
#include "NTFSDataStructures.h"

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
	wmemcpy_s(m_pszFileName, wcslen(pszFileName) + 1, pszFileName, wcslen(pszFileName));
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

ntfs::DeletedFile::~DeletedFile()
{
	free();
}

