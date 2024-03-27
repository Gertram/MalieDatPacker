#include <iostream>
#include <filesystem>
#include <exception>

#include "utils.h"
#include "encryption.h"
#include "Packer.h"
#include "key.h"
#include "cmd.h"


void pack(const std::filesystem::path &dirpath,const std::filesystem::path &filepath,uint32_t align) {
	if (!std::filesystem::is_directory(dirpath)) {
		std::wcout << L"Input must be directory" << std::endl;
		return;
	}
	
	Packer packer(align);
	packer.generate_uncrypted(dirpath,filepath);
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
	std::cout << "[-encrpytion camellia] - sets the encryption type" << std::endl;
	std::cout << "[-align <align>] - sets align for packaging, automatic if key is specified" << std::endl;
	std::cout << "[-dat <*.dat>] - find the key by the header of the .dat file" << std::endl;
	std::cout << "[-game <game>] - find the key by the game name" << std::endl;
	std::cout << "[-expect \"<hex decimal> <hex decimal>\"] - searching for a key by header bytes" << std::endl;
	std::cout << "[-offset <offset>] - sets offset for header bytes to 0x10 by default" << std::endl;
	std::cout << "[-external_key <keypath>] - specifies an external key file" << std::endl;
	std::cout << "[-key <keyname>] - searching for an internal key by name" << std::endl;
	std::cout << "[-encrypt 0|1] - whether to use encryption after the packer" << std::endl;
	std::cout << "[-thread <threads>] - count of thread for encryption" << std::endl;
	std::cout << "[-wait] - wait any key after done" << std::endl;
	std::cout << "-i <input_path> -o <output_path>" << std::endl;
	return 0;
}

int main(int argc, char* argv[]) {
	std::vector<std::string> arguments(argc - 1);

	for (int i = 1; i < argc; i++) {
		arguments.push_back(argv[i]);
	}

	if (arguments.size() == 0) {
		return show_help();
	}

	std::wstring input;

	if (!parseInput(arguments,input)) {
		return -3;
	}
	
	std::wstring output;

	if (!parseOutput(arguments,output)) {
		return -4;
	}

	const auto start = clock();

	const auto config = initConfig(arguments);

	const auto mode = parseWorkMode(arguments);

	if (mode == WorkMode::Pack) {
		const auto useEncrypt = parseUseEncrypt(arguments);

		if (useEncrypt) {
			if (config == nullptr) {
				std::wcout << L"You need key for encryption" << std::endl;
				return -2;
			}
			const auto temppath = L"temp.dat";

			pack(input, temppath, config->getAlign());

			encrypt_file(config->getKey(), temppath, output, parseThread(arguments));

			std::filesystem::remove(temppath);
		}
		else {
			uint32_t align;
			if (config != nullptr) {
				align = config->getAlign();
			}
			else if (!parseAlign(arguments, align)) {
				std::wcout << L"You need align for pack" << std::endl;
				return -5;
			}

			pack(input, output, align);
		}
		
	}
	else if (mode == WorkMode::Encryption) {
		if (config == nullptr) {
			std::wcout << L"You need key for encryption" << std::endl;
			return -2;
		}
		encrypt_file(config->getKey(), input, output, parseThread(arguments));
	}
	else {
		std::wcout << L"Mode undefined" << std::endl;
	}
	const auto end = clock();

	std::cout << "Time: " << float(end - start) / CLOCKS_PER_SEC << std::endl;

	if (contains<std::string>(arguments, "-wait")) {
		getchar();
	}

	return 0;
}