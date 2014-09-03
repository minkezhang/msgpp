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
			Message(std::string ip, std::string hostname, size_t port, std::string message);

			std::string get_ip();
			std::string get_hostname();
			size_t get_port();
			std::string get_message();

		private:
			std::string ip;
			std::string hostname;
			size_t port;

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
			size_t push(std::string message, std::string hostname, size_t port);
			std::string pull(std::string hostname, size_t port);

			// cf. http://bit.ly/1nqOnyd
			static void term(int p);

			static const uint8_t ipv4 = 0;
			static const uint8_t ipv6 = 1;

		private:
			uint8_t protocol;
			size_t port;
			size_t timeout;

			std::shared_ptr<std::atomic<bool>> flag;

			std::vector<Message> messages;

			std::mutex messages_l;

			static std::vector<std::shared_ptr<MessageNode>> instances;
			static std::mutex l;

			static std::chrono::milliseconds increment;
			static const size_t size = 256;
	};
}

#endif

