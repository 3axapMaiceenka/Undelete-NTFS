#pragma once

#include <fstream>
#include <string>
#include <filesystem>

namespace ntfs
{

	class TempFile
	{
	public:
		TempFile(const std::string& fileName, const std::string& relativePath, std::ios::openmode openMode)
			: tempPath(std::filesystem::temp_directory_path()),
			filename(fileName),
			relativeFilePath(relativePath),
			mode(openMode)
		{
			std::filesystem::create_directories(tempPath / relativePath);
			file.open(std::filesystem::path(tempPath / relativePath / fileName), mode);
			if (!file)
			{
				throw std::runtime_error("Can't open temporary file!");
			}
		}

		~TempFile()
		{
			file.close();
			std::filesystem::remove_all(tempPath / relativeFilePath);
		}

		template <typename ... Args>
		std::fstream& output(Args ... args)
		{
			if (!file)
			{
				throw std::runtime_error("Attempt to write to a closed file!");
			}
			if (!(mode & std::ios::out))
			{
				throw std::logic_error("Attempt to write a to read-only file!");
			}

			(file << ... << args);
			return file;
		}

		template <typename ... Args>
		std::fstream& input(Args& ... args)
		{
			if (!file)
			{
				throw std::runtime_error("Attempt to read from a closed file!");
			}
			if (!(mode & std::ios::in))
			{
				throw std::logic_error("Attempt to read from a file opened to write!");
			}

			(file >> ... >> args);
			return file;
		}

		const std::wstring getFilePath() const
		{
			return std::filesystem::path(tempPath / relativeFilePath / filename).native();
		}

	private:
		std::filesystem::path tempPath;
		std::string filename;
		std::string relativeFilePath;
		std::ios::openmode mode;
		std::fstream file;
	};

}