#include <iostream>
#include <cstdio>

#include "utils.h"

void ToShiftJis(const std::wstring& str, char* dst, size_t dst_size) {
	int nJisLen = ::WideCharToMultiByte(932, 0, str.c_str(), static_cast<int>(str.size()), NULL, 0, NULL, NULL);
	if (nJisLen > dst_size) {
		throw std::exception("nJisLen > dst_size");
	}
	::WideCharToMultiByte(932, 0, str.c_str(), static_cast<int>(str.size()), dst, nJisLen, NULL, NULL);
}

void cut_file(const wchar_t* filename, const wchar_t* output_filename, size_t file_size) {
	FILE* input;
	auto result = _wfopen_s(&input, filename, L"rb");
	if (result != 0) {
		throw std::exception("file open error");
	}
	FILE* output;
	result = _wfopen_s(&output, output_filename, L"wb");
	if (result != 0) {
		throw std::exception("file create error");
	}
	char buffer[4096];
	size_t offset = 0;
	size_t count;
	while (!feof(input) && offset <= file_size) {
		count = fread(buffer, sizeof(char), sizeof(buffer), input);
		fwrite(buffer, sizeof(char), count, output);
		offset += sizeof(buffer);
	}

	fclose(input);
	fclose(output);
}

std::vector<std::string_view> splitString(const std::string& str, const std::string& delimiter) {
	std::vector<std::string_view> tokens;
	size_t start = 0, end = 0;
	while ((end = str.find_first_of(delimiter, start)) != std::string::npos) {
		tokens.push_back(str.substr(start, end - start));
		start = end + 1;
	}
	tokens.push_back(str.substr(start));
	return tokens;
}

long long file_compare(const wchar_t *filepath1,const wchar_t *filepath2) {
	FILE *file1,*file2;
	_wfopen_s(&file1, filepath1, L"rb");
	_wfopen_s(&file2, filepath2, L"rb");

	if (file1 == nullptr) {
		return -2;
	}
	if (file2 == nullptr) {
		return -3;
	}

	uint8_t buff1[4096];
	uint8_t buff2[4096];

	size_t offset = 0;
	size_t count1,count2;
	while (!feof(file1)) {
		count1 = fread(buff1,sizeof(uint8_t),sizeof(buff1),file1);
		count2 = fread(buff2,sizeof(uint8_t),sizeof(buff2),file2);
		if (count1 != count2) {
			std::cout << count1 << " " << count2 << std::endl;
			return -3;
		}
		for (int i = 0; i < sizeof(buff1); i++,offset++) {
			const auto value1 = buff1[i];
			const auto value2 = buff2[i];
			if (value1 != value2) {
				return offset;
			}
		}
	}

	fclose(file1);
	fclose(file2);

	return 0;
}