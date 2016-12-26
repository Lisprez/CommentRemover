#include <iostream>
#include <fstream>
#include <functional>
#include <Windows.h>
#include <string>

enum Pattern {
	DOUBLE_QUOTE,
	LINE_COMMENT,
	SEGMENT_COMMENT,
	OTHERS
};

void StateChange(std::ofstream& fout, Pattern& pattern, char& c, char& last)
{
	switch (pattern) {
		case DOUBLE_QUOTE:
		{
			fout << c;
			if (c == '"')
			{
				pattern = OTHERS;
			}
			break;
		}
		case LINE_COMMENT:
		{
			if (last != '\\' && c == '\n')
			{
				fout << c;
				c = '\0';
				pattern = OTHERS;
			}
			break;
		}
		case SEGMENT_COMMENT:
		{
			if (last == '*' && c == '/')
			{
				c = '\0';
				pattern = OTHERS;
			}
			break;
		}
		case OTHERS:
		{
			if (c == '"')
			{
				fout << c;
				pattern = DOUBLE_QUOTE;
			}
			else if (c == '/')
			{
				if (last == '/')
				{
					pattern = LINE_COMMENT;
				}
			}
			else if (c == '*')
			{
				if (last == '/')
				{
					pattern = SEGMENT_COMMENT;
				}
			}
			else
			{
				if (last == '/')
				{
					fout << last;
				}
				fout << c;
				pattern = OTHERS;
			}
			break;
		}
	}
	last = c;
}

int remove_comment(
	const std::string& withCommentFilePath,
	const std::string& withoutCommentFilePath)
{
	std::ifstream fin(withCommentFilePath);
	std::ofstream fout(withoutCommentFilePath);

	if (!fin) {
		return -1;
	}

	Pattern init_pattern = OTHERS;
	char c;
	char last = '\0';

	while (fin.get(c)) {
		StateChange(fout, init_pattern, c, last);
	}

	return 0;
}


int ApplyFunctionToFile(const std::string& srcFolder, 
						std::function<int(const std::string&, const std::string&)> comment_remover,
						const std::string& dstFolder)
{
	WIN32_FIND_DATA FindFileData;
	HANDLE hFind;
	char l_szTmp[1025] = { 0 };
	std::cout << srcFolder.c_str() << std::endl;
	memcpy(l_szTmp, srcFolder.data(), srcFolder.size());


	char l_szSrcPath[1025] = { 0 };
	char l_szDesPath[1025] = { 0 };
	memcpy(l_szSrcPath, srcFolder.data(), srcFolder.size());
	memcpy(l_szDesPath, dstFolder.data(), dstFolder.size());

	char l_szNewSrcPath[1025] = { 0 };
	char l_szNewDesPath[1025] = { 0 };

	strcat(l_szTmp, "\\*");

	hFind = FindFirstFile(l_szTmp, &FindFileData);

	if (hFind == NULL || hFind == INVALID_HANDLE_VALUE) {
		return FALSE;
	}

	do
	{

		if (FindFileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
		{
			if (strcmp(FindFileData.cFileName, "."))
			{
				if (strcmp(FindFileData.cFileName, ".."))
				{
					printf("The Directory found is %s\n", FindFileData.cFileName);

					sprintf(l_szNewDesPath, "%s\\%s", l_szDesPath, FindFileData.cFileName);

					sprintf(l_szNewSrcPath, "%s\\%s", l_szSrcPath, FindFileData.cFileName);
					CreateDirectory(l_szNewDesPath, NULL);
					ApplyFunctionToFile(l_szNewSrcPath, comment_remover, l_szNewDesPath);
				}
			}
		}
		else
		{
			printf("The File found is %s\n", FindFileData.cFileName);
			char l_szSrcFile[1025] = { 0 };
			char l_szDesFile[1025] = { 0 };
			sprintf(l_szDesFile, "%s\\%s", l_szDesPath, FindFileData.cFileName);
			sprintf(l_szSrcFile, "%s\\%s", l_szSrcPath, FindFileData.cFileName);
			std::string temp(FindFileData.cFileName);
			auto ext_name = temp.substr(temp.find_last_of(".") + 1);
			if (ext_name == "h" || ext_name == "cpp" || ext_name == "c") {
				comment_remover(l_szSrcFile, l_szDesFile);
			} 
			else
			{
				CopyFile(l_szSrcFile, l_szDesFile, TRUE);
			}
		}
	} while (FindNextFile(hFind, &FindFileData));

	FindClose(hFind);
	return TRUE;
}

static bool IsDirExist(const std::string& directoryName)
{
	DWORD file_type = GetFileAttributesA(directoryName.c_str());
	if (file_type == INVALID_FILE_ATTRIBUTES)
	{
		return false;
	}

	if (file_type & FILE_ATTRIBUTE_DIRECTORY)
	{
		return true;
	}

	return false;
}


int main(int argc, char* argv[])
{
	if (argc != 3)
	{
		std::cout << "Usage: rm_comnt src_folder target_folder." << std::endl;
		return -1;
	}

	std::string src{ argv[1] };
	std::string dst{ argv[2] };

	if (!IsDirExist(argv[1]))
	{
		std::cout << "Error: source directory not exists !" << std::endl;
		return -1;
	}

	if (!IsDirExist(argv[2]))
	{
		if (CreateDirectory(dst.c_str(), nullptr) == 0)
		{
			std::cout << "Error: create target directory failed." << std::endl;
			return -1;
		}
	}

	ApplyFunctionToFile(src, std::bind(remove_comment, std::placeholders::_1, std::placeholders::_2), dst);

	return 0;
}
