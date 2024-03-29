#include <iostream>
#include <filesystem>
#include <exception>

#include "utils.h"
#include "encryption.h"
#include "Packer.h"
#include "key.h"
#include "cmd.h"


int encrypt(const std::vector<std::string>& arguments, const std::filesystem::path& input, const std::filesystem::path& output) {
	ConfigItem config;

	if (!parseConfig(arguments, config)) {
		std::wcout << L"You need key for decryption" << std::endl;
		return -2;
	}

	uint32_t thread_count;
	if (parseThread(arguments,thread_count) < 0) {
		return -3;
	}

	encrypt_file(config.encryption, input, output,thread_count);
	return 0;
}

int decrypt(const std::vector<std::string>& arguments, const std::filesystem::path& input, const std::filesystem::path& output) {
	ConfigItem config;

	if (!parseConfig(arguments, config)) {
		std::wcout << L"You need key for decryption" << std::endl;
		return -2;
	}
	uint32_t thread_count;
	if (parseThread(arguments, thread_count) < 0) {
		return -3;
	}
	decrypt_file(config.encryption, input, output, parseThread(arguments,thread_count));
	return 0;
}


int pack(const std::vector<std::string> &arguments,const std::filesystem::path &dirpath,const std::filesystem::path &filepath) {
	std::cout << "Try packing" << std::endl;
	if (!std::filesystem::is_directory(dirpath)) {
		std::wcout << L"Input must be directory" << std::endl;
		return -6;
	}

	bool useEncrypt;
	
	if (parseUseEncrypt(arguments,useEncrypt) < 0) {
		return -7;
	}
	ConfigItem config;

	const auto isConfigLoad = parseConfig(arguments, config) >= 0;


	if (!useEncrypt) {
		uint32_t align;
		if (isConfigLoad) {
			align = config.align;
		}
		else if (!parseAlign(arguments, align)) {
			std::wcout << L"You need align for pack" << std::endl;
			return -5;
		}

		Packer packer(align);

		packer.generate_uncrypted(dirpath, filepath);
	}
	else {
		if (!isConfigLoad) {
			std::wcout << L"You need key for encryption" << std::endl;
			return -2;
		}
		Packer packer(config.align);

		bool notemp;

		if (parseNoTemp(arguments,notemp) < 0) {
			return -3;
		}
		uint32_t thread_count;
		if (parseThread(arguments, thread_count) < 0) {
			return -3;
		}
		if (notemp) {

			size_t filesize;
			uint8_t* data;
			packer.generate_uncrypted_in_memory(dirpath,&data,filesize);
			encrypt_memory(config.encryption, data, filesize, thread_count);
			FILE* file;
			if (_wfopen_s(&file, filepath.c_str(), L"wb") != 0 || file == nullptr) {
				std::wcout << "Create file error" << std::endl;
				return -10;
			}
			fwrite(data, sizeof(uint8_t), filesize, file);
			fclose(file);
		}
		else {

			const auto temppath = L"temp.dat";

			packer.generate_uncrypted(dirpath, temppath);

			encrypt_file(config.encryption, temppath, filepath, thread_count);

			std::filesystem::remove(temppath);
		}
	}

	
	std::cout << "Done" << std::endl;
	return 0;
}

void print_games(const std::vector<std::wstring>& games) {
	std::wcout << L"Found config for games:" << std::endl;
	for (size_t i = 0; i < games.size(); i++) {
		std::wcout << (i + 1) << L"." << games[i] << std::endl;
	}
}

int show_help() {
	std::cout << "Usage:" << std::endl;
	std::cout << "[-mode pack|encrypt] - sets the operating mode of the packer or encryptor" << std::endl;
	std::cout << "[-align <align>] - sets align for packaging, automatic if key is specified" << std::endl;
	std::cout << "[-dat <*.dat>] - find the key by the header of the .dat file" << std::endl;
	std::cout << "[-game <game>] - find the key by the game name" << std::endl;
	std::cout << "[-expect \"<hex decimal> <hex decimal>\"] - searching for a key by header bytes" << std::endl;
	std::cout << "[-offset <offset>] - sets offset for header bytes to 0x10 by default" << std::endl;
	std::cout << "[-external_key <keypath>] - specifies an external key file" << std::endl;
	std::cout << "[-internal_key <keyname>] - searching for an internal key by name" << std::endl;
	std::cout << "[-key <keyname> -encryption <camellia|malie> -align <align>] - set key, encryption and align" << std::endl;
	std::cout << "[-encrypt 0|1] - whether to use encryption after the packer" << std::endl;
	std::cout << "[-notemp 0|1] - whether to use temp files. Need so much memory!" << std::endl;
	std::cout << "[-thread <threads>] - count of thread for encryption" << std::endl;
	std::cout << "[-wait] - wait any key after done" << std::endl;
	std::cout << "-i <input_path> -o <output_path>" << std::endl;
	getchar();
	return 0;
}

int work(std::vector<std::string> &arguments) {
	//arguments.push_back("-mode");
	////arguments.push_back("encrypt");
	//arguments.push_back("pack");
	//arguments.push_back("-wait");
	//arguments.push_back("-notemp");
	//arguments.push_back("1");
	//arguments.push_back("-encrypt");
	//arguments.push_back("1");
	//arguments.push_back("-internal_key");	
	//arguments.push_back("1");
	//arguments.push_back("-thread");
	//arguments.push_back("4");
	//arguments.push_back("-i");
	////arguments.push_back("D:\\Games\\danzai\\data\\game");
	////arguments.push_back("Z:\\temp.dat");
	//arguments.push_back("Z:\\game");
	//arguments.push_back("-o");
	////arguments.push_back("D:\\Games\\danzai\\data\\game.dat");
	//arguments.push_back("Z:\\game.dat");
	////arguments.push_back("Z:\\temp.dat");
	if (arguments.size() == 0) {
		return show_help();
	}
	std::wstring input;

	if (!parseInput(arguments, input)) {
		return -3;
	}

	std::wstring output;

	if (!parseOutput(arguments, output)) {
		return -4;
	}

	WorkMode mode;
	
	if (parseWorkMode(arguments, mode) < 0) {
		return -5;
	}

	if (mode == WorkMode::Pack) {
		return pack(arguments,input,output);
	}
	if (mode == WorkMode::Encryption) {
		return encrypt(arguments, input, output);
	}
	if (mode == WorkMode::Decryption) {
		return decrypt(arguments, input, output);
	}
	std::wcout << L"Mode undefined" << std::endl;

	return 0;
}

int main(int argc, char* argv[]) {
	/*file_compare(L"Z:/game1.dat", L"Z:/game.dat");
	return 0;*/
	std::vector<std::string> arguments(argc - 1);

	for (int i = 1; i < argc; i++) {
		arguments.push_back(argv[i]);
	}

	for (int i = 0; i < 1; i++) {

		try {
			const auto start = clock();

			const auto result = work(arguments);
			if (result < 0) {
				std::wcout << "Error code is " << result << std::endl;
				getchar();
				return 0;
			}
			const auto end = clock();

			std::cout << "Time: " << float(end - start) / CLOCKS_PER_SEC << std::endl;
		}
		catch (std::exception& exp) {
			std::cout << "Worker error:" << exp.what() << std::endl;
		}
	}
	if (contains<std::string>(arguments, "-wait")) {
		getchar();
	}

	return 0;
}