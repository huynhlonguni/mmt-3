#include <asio.hpp>
#include "types.h"

class MulticastServer {
public:
	MulticastServer(asio::io_context& io_context, const asio::ip::address& multicast_address, const std::vector<std::string> &local_ips)
		: endpoint_(multicast_address, MULTICAST_PORT)
		, socket_(io_context, endpoint_.protocol())
		, timer_(io_context) {
		size = ip_to_bytes(local_ips, &data);
		do_send();
	}

private:
	int ip_to_bytes(const std::vector<std::string> local_ips, char **data) {
		int totalSize = 0;
		for (const auto &str: local_ips) {
			totalSize += str.size() + 1;  // +1 for the null terminator
		}

		char *rawData = new char[totalSize];

		int offset = 0;
		for (const auto &str: local_ips) {
			std::copy(str.begin(), str.end(), rawData + offset);
			rawData[offset + str.size()] = '\0';  // Add null terminator
			offset += str.size() + 1;
		}
		*data = rawData;
		return totalSize;
	}

	void do_send() {
		socket_.async_send_to(asio::buffer(data, size), endpoint_,
			[this](std::error_code ec, std::size_t /*length*/) {
				if (!ec)
					do_timeout();
			});
	}

	void do_timeout() {
		timer_.expires_after(std::chrono::seconds(MULTICAST_INTERVAL_SECONDS));
		timer_.async_wait([this](std::error_code ec) {
			if (!ec)
				do_send();
		});
	}

private:
	asio::ip::udp::endpoint endpoint_;
	asio::ip::udp::socket socket_;
	asio::steady_timer timer_;
	char *data;
	int size;
};
