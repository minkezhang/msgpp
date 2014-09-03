#ifndef _MSGPP_MSG_NODE_H
#define _MSGPP_MSG_NODE_H

#include <atomic>
#include <chrono>
#include <memory>
#include <mutex>
#include <string>
#include <vector>

namespace msgpp {
	class Message {
		public:
			Message(std::string ip, std::string hostname, std::string message);

			std::string get_ip();
			std::string get_hostname();
			std::string get_message();

		private:
			std::string ip;
			std::string hostname;

			std::string message;
	};

	class MessageNode : public std::enable_shared_from_this<MessageNode> {
		public:
			MessageNode(size_t port, uint8_t protocol = ipv4, size_t timeout = 2);

			uint8_t get_protocol();
			size_t get_port();
			size_t get_timeout();
			void set_timeout(size_t timeout);

			// start a persistent incoming socket
			void up();
			void dn();

			// send to a persistent endpoint
			size_t push(std::string message, std::string hostname, size_t port, bool silent_fail = false);
			std::string pull(std::string hostname = "", bool silent_fail = false);

			// get number of messages currently in queue
			size_t query();

			// SIGINT handler
			// cf. http://bit.ly/1nqOnyd
			static void term(int p);

			static const uint8_t ipv4 = 0;
			static const uint8_t ipv6 = 1;

		private:
			uint8_t protocol;
			size_t port;
			size_t timeout;

			std::vector<std::shared_ptr<std::thread>> threads;
			std::shared_ptr<std::atomic<bool>> flag;

			std::vector<Message> messages;
			std::mutex messages_l;

			void dispatch(int client_sock, struct sockaddr_storage client_addr, socklen_t client_size);

			static std::vector<std::shared_ptr<MessageNode>> instances;
			static std::mutex l;

			static std::chrono::milliseconds increment;
			static const size_t size = 1024;
			static const size_t max_conn = 100;
	};
}

#endif

