#pragma once

#include <string>
#include <exception>
#include <vector>
#include <Windows.h>


void ToShiftJis(const std::wstring& str, char* dst, size_t dst_size);


void cut_file(const wchar_t* filename, const wchar_t* output_filename, size_t file_size);

long long file_compare(const wchar_t* filepath1, const wchar_t* filepath2);

#ifndef UTILS_H
#define UTILS_H

template<typename T>
bool contains(const std::vector<T>& arr, const T& str) {
	return std::find(arr.begin(), arr.end(), str) != arr.end();
}

template<typename T>
int findPosition(const std::vector<T>& vec, const T& target) {
	auto it = std::find(vec.begin(), vec.end(), target);
	if (it != vec.end()) {
		return static_cast<int>(std::distance(vec.begin(), it));
	}
	return -1;
}

std::vector<std::string_view> splitString(const std::string& str, const std::string& delimiter);

#endif