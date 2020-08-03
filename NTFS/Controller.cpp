#include "Controller.h"

ntfs::Controller::Controller()
	: m_pMftParsers(std::make_unique<std::vector<MFTParser>>()),
	  m_pDrivesInfo(std::make_shared<std::vector<MFTInfo>>())
{ }

BOOLEAN ntfs::Controller::start()
{
	try
	{
		PartitionTableParser parser;
		parser.parse();

		try
		{
			ntfs::DrivesInfo di(parser.getLogicalDrives());
			di.getDrivesInfo();
			m_pDrivesInfo = di.getDrivesMFT();

			for (auto& mftInfo : *m_pDrivesInfo)
			{
				m_pMftParsers->emplace_back(std::move(MFTParser(mftInfo)));
			}
		}
		catch (std::runtime_error error)
		{
			std::cout << error.what() << std::endl;
			return FALSE;
		}
	}
	catch (std::runtime_error error)
	{
		std::cout << error.what() << std::endl;
		return FALSE;
	}

	return TRUE;
}

const std::shared_ptr<ntfs::VolumeInfo> ntfs::Controller::getVolumeInfo(std::size_t volumeNumb) const
{
	return m_pMftParsers->at(volumeNumb).getVolumeInfo();
}

const std::shared_ptr<std::vector<ntfs::MFTInfo>> ntfs::Controller::getDrivesInfo() const
{
	return m_pDrivesInfo;
}

void ntfs::Controller::findDeletedFiles(std::size_t volumeNumb)
{
	m_pMftParsers->at(volumeNumb).findDeletedFiles();
}

const std::shared_ptr<std::list<ntfs::DeletedFile>> ntfs::Controller::getDeletedFiles(std::size_t volumeNumb) const
{
	return m_pMftParsers->at(volumeNumb).getDeletedFiles();
}

void ntfs::Controller::undelete(std::size_t volumeNumb, std::size_t fileNumb, const std::wstring& directory) const
{
	auto deletedFiles = m_pMftParsers->at(volumeNumb).getDeletedFiles();
	m_pMftParsers->at(volumeNumb).undelete(&(*std::next(deletedFiles->begin(), fileNumb)), directory.c_str());
}

std::size_t ntfs::Controller::getNumberOfVolumes() const
{
	return m_pMftParsers->size();
}



