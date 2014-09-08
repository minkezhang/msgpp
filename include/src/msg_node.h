#ifndef _MSGPP_MSG_NODE_H
#define _MSGPP_MSG_NODE_H

#include <atomic>
#include <chrono>
#include <memory>
#include <mutex>
#include <string>
#include <sys/socket.h>
#include <signal.h>
#include <thread>
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
			/**
			 * protocol -- set to msgpp::MessageNode::ipv6 to support IPv6 addresses
			 *	else, set set ai_family = AF_UNSPEC in ::up
			 * timeout -- instance time metric (in seconds) to handle non-blocking sockets
			 */
			MessageNode(size_t port = 0, uint8_t protocol = ipv4, size_t timeout = 2, size_t max_conn = 100);

			uint8_t get_protocol();
			size_t get_port();
			size_t get_timeout();
			size_t get_max_conn();

			/**
			 * returns 1 if the node is currently running the server (i.e. if MessageNode::up has been invoked)
			 */
			bool get_status();

			void set_timeout(size_t timeout);

			/**
			 * start a persistent socket at localhost:PORT
			 */
			void up();

			/**
			 * shutdown the server and close all stray connections
			 */
			void dn();

			/**
			 * push MESSAGE to remote HOSTNAME:PORT endpoint
			 *
			 * silent_fail -- return 0 on failure instead of throwing exceptionpp::RuntimeError
			 */
			size_t push(std::string message, std::string hostname, size_t port, bool silent_fail = false);

			/**
			 * pull from the message queue filtered by HOSTNAME
			 *
			 * silent_fail -- return "" on failure instead of throwing exceptionpp::RuntimeError
			 */
			std::string pull(std::string hostname = "", bool silent_fail = false);

			/**
			 * get number of messages currently in queue
			 */
			size_t query();

			/**
			 * SIGINT handler
			 *	cf. http://bit.ly/1nqOnyd
			 */
			static void term(int p);

			static const uint8_t ipv4 = 1;
			static const uint8_t ipv6 = 2;

		private:
			uint8_t protocol;
			size_t port;
			size_t timeout;
			size_t max_conn;

			std::vector<std::shared_ptr<std::thread>> threads;
			std::shared_ptr<std::atomic<bool>> flag;

			std::vector<Message> messages;
			std::mutex messages_l;

			void dispatch(int client_sock, struct sockaddr_storage client_addr, socklen_t client_size);

			static std::vector<std::shared_ptr<MessageNode>> instances;
			static std::mutex l;

			static std::chrono::milliseconds increment;
			static const size_t size = 1024;
			static sighandler_t handler;
	};
}

#endif

