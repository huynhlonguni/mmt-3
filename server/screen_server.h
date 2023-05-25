#include <iostream>
#include <string>
#include <memory>
#include <functional>
#include <asio.hpp>
#include "types.h"
using asio::ip::tcp;

class ScreenConnection : public std::enable_shared_from_this<ScreenConnection> {
public:
	typedef std::shared_ptr<ScreenConnection> pointer;

	static pointer create(asio::io_context& io_context, const std::function<FrameData(bool)> &callback) {
		return pointer(new ScreenConnection(io_context, callback));
	}

	tcp::socket& socket() {
		return socket_;
	}

	void start(const ScreenInfo &info) {
		asio::error_code ignored_error;
		asio::async_write(socket_, asio::buffer(&info, sizeof(info)),
			std::bind(&ScreenConnection::handle_write, shared_from_this(), std::placeholders::_1 /*error*/));
	}

private:
	ScreenConnection(asio::io_context& io_context, const std::function<FrameData(bool)> &callback)
	: socket_(io_context)
	, callback_(callback) {}

	void write(void *data, int size) {
		asio::error_code error;
		asio::write(socket_, asio::buffer(data, size), error);
	}

	void handle_write(asio::error_code error) {
		if (!error) {
			FrameData buffer = callback_(init);
			if (buffer.bd.screen_changed) {
				write(&buffer.bd, sizeof(FrameBundle));
				if (buffer.bd.mouse_changed) {
					write(buffer.mouse_data, buffer.bd.mouse_size);
				}
				write(buffer.keys_pressed.data(), buffer.bd.num_keys_pressed);
				write(buffer.screen_data, buffer.bd.screen_size);
				OperationCode opcode = FRAME_DATA;
				asio::async_write(socket_, asio::buffer(&opcode, sizeof(OperationCode)),
					std::bind(&ScreenConnection::handle_write, shared_from_this(), std::placeholders::_1));
			}
			else {
				socket_.async_wait(asio::ip::tcp::socket::wait_write,
					std::bind(&ScreenConnection::handle_write, shared_from_this(), std::placeholders::_1));
			}
			init = 0;
		}
	}

	tcp::socket socket_;
	bool init = 1;
	const std::function<FrameData(bool)> callback_;
};


class ScreenServer {
public:
	ScreenServer(asio::io_context& io_context, const ScreenInfo &info, const std::function<FrameData(bool)> &callback)
		: io_context_(io_context)
		, acceptor_(io_context, tcp::endpoint(tcp::v4(), SOCKET_SCREEN_PORT)) {
		start_accept(info, callback);
	}

private:
	void start_accept(const ScreenInfo &info, const std::function<FrameData(bool)> &callback) {
		ScreenConnection::pointer new_connection = ScreenConnection::create(io_context_, callback);

		acceptor_.async_accept(new_connection->socket(),
				std::bind(&ScreenServer::handle_accept, this, info, callback, new_connection, std::placeholders::_1 /*error*/));
	}

	void handle_accept(const ScreenInfo &info,
						const std::function<FrameData(bool)> &callback,
						ScreenConnection::pointer new_connection,
						const asio::error_code& error) {
		if (!error) {
			new_connection->start(info);
		}
		start_accept(info, callback);
	}

	asio::io_context& io_context_;
	tcp::acceptor acceptor_;
};