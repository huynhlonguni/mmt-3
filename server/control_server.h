#include <iostream>
#include <string>
#include <memory>
#include <functional>
#include <thread>
#include <asio.hpp>
#include "types.h"
#include "processes.h"
#include "fs.h"
using asio::ip::tcp;

FileSystemWorker fs_worker;
std::thread fs_worker_thread(fs_worker.execute, std::ref(fs_worker));

class ControlConnection : public std::enable_shared_from_this<ControlConnection> {
public:
	typedef std::shared_ptr<ControlConnection> pointer;

	static pointer create(asio::io_context& io_context, const std::function<void()> &callback) {
		return pointer(new ControlConnection(io_context, callback));
	}

	tcp::socket& socket() {
		return socket_;
	}

	void start() {
		asio::async_read(socket_, asio::buffer(&opcode, sizeof(OperationCode)), 
			std::bind(&ControlConnection::handle_read, shared_from_this(), std::placeholders::_1));
	}

private:
	ControlConnection(asio::io_context& io_context, const std::function<void()> &callback)
	: socket_(io_context)
	, callback_(callback) {}

	template <class T>
	T read() {
		T obj;
		asio::read(socket_, asio::buffer(&obj, sizeof(obj)));
		return obj;
	}

	std::string readString() {
		int size = read<int>();
		std::string str;
		str.resize(size);
		asio::read(socket_, asio::buffer(str, size));
		return str;
	}

	void readData(void *data, int size) {
		asio::error_code error;
		asio::read(socket_, asio::buffer(data, size));
	}

	void write(void *data, int size) {
		asio::error_code error;
		asio::write(socket_, asio::buffer(data, size), error);
	}

	void write(std::string str) {
		asio::error_code error;
		int size = str.size();
		asio::write(socket_, asio::buffer(&size, sizeof(int)), error);
		asio::write(socket_, asio::buffer(str, size), error);
	}

	void handle_read(const asio::error_code& error) {
		if (!error) {
			switch (opcode) {
				// case CONTROL_DISCONNECT:
				// 	callback_();
				// 	break;
				case PROCESS_LIST: {
					//std::cout << "PROCESS_LIST" << std::endl;
					auto processes = get_current_processes();
					int size = processes.size();
					write(&size, sizeof(int));
					for (auto &process: processes) {
						write(&process.pid, sizeof(int));
						write(process.name);
						write(&process.type, sizeof(char));
					}
					break;
				}
				case PROCESS_SUSPEND: {
					int pid = read<int>();
					//std::cout << "PROCESS_SUSPEND: " << pid << std::endl;
					suspend_process(pid);
					break;
				}
				case PROCESS_RESUME: {
					int pid = read<int>();
					//std::cout << "PROCESS_RESUME: " << pid << std::endl;
					resume_process(pid);
					break;
				}
				case PROCESS_KILL: {
					int pid = read<int>();
					//std::cout << "PROCESS_KILL: " << pid << std::endl;
					terminate_process(pid);
					break;
				}
				case FS_INIT: {
					//std::cout << "FS_INIT" << std::endl;
					write(filesystem_get_default_location());
					break;
				}
				case FS_LIST: {
					std::string path = readString();
					//std::cout << "FS_LIST: " << path << std::endl;
					auto files_list = filesystem_list(path);
					int size = files_list.size();
					write(&size, sizeof(int));
					for (auto &entry: files_list) {
						write(entry.name);
						write(&entry.type, sizeof(char));
					}
					break;
				}
				case FS_CHECK_EXIST: {
					std::string from = readString();
					std::string to = readString();
					//std::cout << "FS_CHECK_EXIST: " << from << " | " << to << std::endl;
					bool exist = filesystem_check_exist(from, to);
					write(&exist, sizeof(bool));
					break;
				}
				case FS_COPY: {
					std::string from = readString();
					std::string to = readString();
					bool overwrite = read<bool>();
					//std::cout << "FS_COPY: " << from << " | " << to << " | ";
					if (overwrite) std::cout << "Overwrite" << std::endl;
					else std::cout << "Skip" << std::endl;
					fs_worker.queueCopy(from, to, overwrite, [](std::error_code error) {
						if (error)
							std::cout << "Error during copy: " << error.message() << std::endl;
					});
					break;
				}
				case FS_MOVE: {
					std::string from = readString();
					std::string to = readString();
					bool overwrite = read<bool>();
					//std::cout << "FS_MOVE: " << from << " | " << to << " | ";
					if (overwrite) std::cout << "Overwrite" << std::endl;
					else std::cout << "Skip" << std::endl;
					fs_worker.queueMove(from, to, overwrite, [](std::error_code error) {
						if (error)
							std::cout << "Error during move: " << error.message() << std::endl;
					});
					break;
				}
				case FS_WRITE: {
					std::string path = readString();
					bool overwrite = read<bool>();
					int size = read<int>();
					//std::cout << "FS_WRITE: " << path << " | " << size << " | ";
					if (overwrite) std::cout << "Overwrite" << std::endl;
					else std::cout << "Skip" << std::endl;
					std::shared_ptr<char[]> data_buffer(new char[size]);
					readData(data_buffer.get(), size);
					fs_worker.queueWrite(path, overwrite, size, data_buffer, [](std::error_code error) {
						if (error)
							std::cout << "Error during write" << std::endl;
					});
					break;
				}
				case FS_DELETE: {
					std::string path = readString();
					//std::cout << "FS_DELETE: " << path << std::endl;
					fs_worker.queueDelete(path, [](std::error_code error) {
						if (error)
							std::cout << "Error during delete: " << error.message() << std::endl;
					});
					break;
				}
			}
			asio::async_read(socket_, asio::buffer(&opcode, sizeof(OperationCode)), 
				std::bind(&ControlConnection::handle_read, shared_from_this(), std::placeholders::_1));
		}
	}

	void handle_write(asio::error_code error) {
		if (!error) {

		}
	}

	void void_write(asio::error_code error) {}

	tcp::socket socket_;
	int mouse_x = 0;
	int mouse_y = 0;
	const std::function<void()> callback_;
	OperationCode opcode;
};

class ControlServer {
public:
	ControlServer(asio::io_context& io_context, const std::function<void()> &callback)
		: io_context_(io_context)
		, acceptor_(io_context, tcp::endpoint(tcp::v4(), SOCKET_CONTROL_PORT)) {
		start_accept(callback);
	}

private:
	void start_accept(const std::function<void()> &callback) {
		ControlConnection::pointer new_connection = ControlConnection::create(io_context_, callback);

		acceptor_.async_accept(new_connection->socket(),
				std::bind(&ControlServer::handle_accept, this, callback, new_connection, std::placeholders::_1 /*error*/));
	}

	void handle_accept(const std::function<void()> &callback, ControlConnection::pointer new_connection, const asio::error_code& error) {
		if (!error) {
			new_connection->start();
		}
		start_accept(callback);
	}

	asio::io_context& io_context_;
	tcp::acceptor acceptor_;
};