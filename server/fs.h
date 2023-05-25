#include <filesystem>
#include <string>
#include <fstream>
#include <memory>
#include <vector>
#include "queue.h"
#include <functional>
#include <system_error>
namespace fs = std::filesystem;

fs::path filesystem_get_unicode_path(std::string path) {
	static std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;
	std::wstring wide = converter.from_bytes(path.c_str());
	return fs::path(wide);
}

std::vector<char> filesystem_get_drives() {
	std::vector<char> drives;
	for (char letter = 'A'; letter <= 'Z'; ++letter) {
		fs::path drive_path = fs::path(std::string(1, letter) + ":\\");
		if (fs::exists(drive_path)) {
			drives.push_back(letter);
		}
	}
	return drives;
}

std::string filesystem_get_default_location() {
	return std::string(getenv("USERPROFILE"));
}

std::vector<FileInfo> filesystem_list(std::string path) {
	if (path == "") { //Drives
		auto drive_letters = filesystem_get_drives();
		std::vector<FileInfo> drives;
		for (const auto &letter: drive_letters)
			drives.push_back({std::string(1, letter), ENTRY_DRIVE});
		return drives;
	} //else

	fs::path origin = filesystem_get_unicode_path(path);
	std::vector<FileInfo> folders(1, {"..", ENTRY_PARENT});
	std::vector<FileInfo> files;
	std::error_code error;
	for (const auto& entry: fs::directory_iterator(origin, error)) {
		if (fs::is_directory(entry.path()))
			folders.push_back({entry.path().filename().string(), ENTRY_FOLDER});
		else 
			files.push_back({entry.path().filename().string(), ENTRY_FILE});
	}
	//Folders first, then files
	folders.insert(folders.end(), files.begin(), files.end());
	
	return folders;
}

bool is_folder(const fs::path &path) {
	return fs::exists(path) && fs::is_directory(path);
}

bool is_file(const fs::path &path) {
	//Clump symlink and stuff together as file. Further error will be processed by move/copy operation
	return fs::exists(path) && !fs::is_directory(path);

	//return fs::exists(filePath) && fs::is_regular_file(filePath);
}

bool filesystem_check_exist_file(const fs::path &from, const fs::path &to) {
	if (is_file(to)) {
		//std::cout << "Error: File with the same name already exists in the destination directory." << std::endl;
		return 1;
	}
	return 0;
}

//Whether the destination to already has sub-files of the same name as from's sub-files
bool filesystem_check_exist_dir(const fs::path &from, const fs::path &to) {
	for (const auto& entry : fs::recursive_directory_iterator(from)) {
		if (is_file(entry.path())) {
			fs::path entry_path = to / entry.path().lexically_relative(from);
			if (is_file(entry_path)) {
				//std::cout << "Error: File with the same name already exists in the destination folder." << std::endl;
				return 1;
			}
		}
	}
	return 0;
}

bool filesystem_check_exist(const std::string &from, const std::string &to) {
	fs::path from_path = filesystem_get_unicode_path(from);
	fs::path to_path = filesystem_get_unicode_path(to);

	if (is_file(from_path)) {
		return filesystem_check_exist_file(from_path, to_path);
	}
	else if (is_folder(from_path)) {
		return filesystem_check_exist_dir(from_path, to_path);
	}

	return 0;
}

//parent should not end with slash
std::error_code filesystem_copy(const std::string &from, const std::string &to, bool overwrite) {
	//copy_options is apparently broken on MinGW (overwrite_existing option is not accounted for). Do it manually:
	//Delete file if exists and overwrite = true and copy.
	std::error_code error;

	fs::path from_path = filesystem_get_unicode_path(from);
	fs::path to_path = filesystem_get_unicode_path(to);

	if (is_folder(from_path) && is_folder(to_path)) {
		for (const auto& entry : fs::recursive_directory_iterator(from_path)) {
			fs::path to_entry_path = to_path / entry.path().lexically_relative(from_path);
			if (is_file(entry.path())) {
				if (is_file(to_entry_path)) {
					if (overwrite) {
						fs::remove(to_entry_path, error);
						fs::copy(entry.path(), to_entry_path, error);
					}
				}
				else {
					fs::copy(entry.path(), to_entry_path, error);
				}
			}
			else if (is_folder(entry.path())) {
				fs::create_directories(to_entry_path, error);
			}
		}
	}
	else {
		if (is_file(to_path)) {
			if (overwrite) {
				fs::remove(to_path);
				fs::copy(from_path, to_path, error);
			}
		}
		else {
			fs::copy(from_path, to_path, error);
		}
	}
	return error;
}

std::error_code filesystem_rename(const std::string &from, const std::string &to, bool overwrite) {
	std::error_code error;

	fs::path from_path = filesystem_get_unicode_path(from);
	fs::path to_path = filesystem_get_unicode_path(to);

	//fs::rename on folder requires destination folder to not exist or empty to perform simple relink.
	//Windows allow merging when doing it via Explorer. This part replicate it
	if (is_folder(from_path) && is_folder(to_path)) {
		for (const auto& entry : fs::recursive_directory_iterator(from_path)) {
			fs::path to_entry_path = to_path / entry.path().lexically_relative(from_path);
			if (is_file(entry.path())) {
				//std::cout << "Entry " << to_entry_path << std::endl;
				if (is_file(to_entry_path)) {
					if (overwrite) {
						fs::remove(to_entry_path, error);
						fs::rename(entry.path(), to_entry_path, error);
					}
				}
				else {
					fs::rename(entry.path(), to_entry_path, error);
				}
			}
			else if (is_folder(entry.path())) {
				fs::create_directories(to_entry_path, error);
			}
		}
	}
	else {
		if (is_file(to_path)) {
			if (overwrite) {
				fs::remove(to_path);
				fs::rename(from_path, to_path, error);
			}
		}
		else {
			fs::rename(from_path, to_path, error);
		}
	}
	return error;
}

std::error_code filesystem_write(const std::string &path, bool overwrite, bool append, const char data[], size_t size) {
	fs::path file_path = filesystem_get_unicode_path(path);

	if (is_file(file_path))
		if (!overwrite) return std::make_error_code(std::errc::file_exists);

	std::ofstream file_stream;
	file_stream.open(file_path, std::ios::binary);

	if (file_stream.fail()) return std::make_error_code(std::errc::io_error);

	file_stream.write(data, size);
	std::error_code no_error;
	return no_error;
}

std::error_code filesystem_delete(const std::string &path) {
	fs::path file_path = filesystem_get_unicode_path(path);

	std::error_code error;
	bool num_deleted = fs::remove_all(file_path, error);
	return error;
}

enum FSJobType : char {
	FS_JOB_NONE = 0,
	FS_JOB_COPY,
	FS_JOB_MOVE,
	FS_JOB_WRITE,
	FS_JOB_DELETE
};

struct FSJob {
	FSJobType type;
	std::string from;
	std::string to;
	bool overwrite;
	std::function<void(std::error_code)> callback;
	int data_size;
	std::shared_ptr<char[]> data_ptr;
};

class FileSystemWorker {
private:
	SharedQueue<FSJob> jobs; //practically this application should wait for job to finish first before sending another, if there're any
	bool is_stopped = 0;
public:
	void execute() {
		while (1) {
			if (jobs.empty()) std::this_thread::sleep_for(std::chrono::seconds(1));
			else {
				std::error_code error;
				FSJob job = jobs.front();
				jobs.pop_front();
				switch (job.type) {
					case FS_JOB_NONE: break;
					case FS_JOB_COPY:
						error = filesystem_copy(job.from, job.to, job.overwrite);
						job.callback(error);
						break;
					case FS_JOB_MOVE:
						error = filesystem_rename(job.from, job.to, job.overwrite);
						job.callback(error);
						break;
					case FS_JOB_WRITE:
						error = filesystem_write(job.from, job.overwrite, 0, job.data_ptr.get(), job.data_size);
						job.callback(error);
						break;
					case FS_JOB_DELETE:
						error = filesystem_delete(job.from);
						job.callback(error);
						break;
				}
			}
		}
	}

	void queueCopy(std::string from, std::string to, bool overwrite, std::function<void(std::error_code)> callback) {
		jobs.push_back({FS_JOB_COPY, from, to, overwrite, callback, 0, nullptr});
	}

	void queueMove(std::string from, std::string to, bool overwrite, std::function<void(std::error_code)> callback) {
		jobs.push_back({FS_JOB_MOVE, from, to, overwrite, callback, 0, nullptr});
	}

	void queueWrite(std::string path, bool overwrite, int size, std::shared_ptr<char[]> data, std::function<void(std::error_code)> callback) {
		jobs.push_back({FS_JOB_WRITE, path, "", overwrite, callback, size, data});
	}

	void queueDelete(std::string path, std::function<void(std::error_code)> callback) {
		jobs.push_back({FS_JOB_DELETE, path, "", 0, callback, 0, nullptr});
	}
};