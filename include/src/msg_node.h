#ifndef _MSGPP_MSG_NODE_H
#define _MSGPP_MSG_NODE_H

#include <atomic>
#include <memory>
#include <mutex>
#include <string>
#include <vector>

namespace msgpp {
	class Message {
		public:
			Message(std::string hostname, size_t port, std::string message);

			std::string get_hostname();
			size_t get_port();
			std::string get_message();

		private:
			std::string hostname;
			size_t port;

			std::string message;
	};

	class MessageNode : public std::enable_shared_from_this<MessageNode> {
		public:
			MessageNode(std::string hostname, size_t port);

			std::string get_hostname();
			size_t get_port();

			// start a persistent incoming socket
			void up();
			void dn();

			// send to a persistent endpoint
			size_t send(std::string message, std::string hostname, size_t port);
			std::string recv(std::string hostname, size_t port);

			// cf. http://bit.ly/1nqOnyd
			static void term(int p);

		private:
			std::string hostname;
			size_t port;
			std::shared_ptr<std::atomic<bool>> flag;

			std::vector<Message> messages;

			std::mutex messages_l;

			static std::vector<std::shared_ptr<MessageNode>> instances;
			static std::mutex l;
	};
}

#endif

