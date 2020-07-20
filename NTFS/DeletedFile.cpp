#include "DeletedFile.h"
#include "MFTParser.h"

ntfs::DeletedFile::DeletedFile()
	: m_uRecordAddress(0),
	m_pFileNameAttr(NULL),
	m_pszFileName(NULL),
	m_cType(0)
{ }

ntfs::DeletedFile::DeletedFile(UINT64 uRecordAddress, const FILE_NAME_ATTR& fileNameAttr, const WCHAR* pszFileName, CHAR cType)
	: m_uRecordAddress(uRecordAddress),
	m_pFileNameAttr(new FILE_NAME_ATTR(fileNameAttr)),
	m_pszFileName(new WCHAR[wcslen(pszFileName) + 1]),
	m_cType(cType)
{
	wmemcpy_s(m_pszFileName, wcslen(pszFileName) + 1, pszFileName, wcslen(pszFileName));
}

ntfs::DeletedFile::DeletedFile(const DeletedFile& rhs)
	: DeletedFile(rhs.m_uRecordAddress, *rhs.m_pFileNameAttr, rhs.m_pszFileName, rhs.m_cType)
{ }

ntfs::DeletedFile::DeletedFile(DeletedFile&& rhs) noexcept
	: m_uRecordAddress(rhs.m_uRecordAddress),
	m_pFileNameAttr(rhs.m_pFileNameAttr),
	m_pszFileName(rhs.m_pszFileName),
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

ntfs::DeletedFile::~DeletedFile()
{
	free();
}

