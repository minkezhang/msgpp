#ifndef _ENTANGLE_MSG_NODE_H
#define _ENTANGLE_MSG_NODE_H

#include <string>

namespace entangle {
	class MessageNode {
		public:
			MessageNode(std::string hostname, size_t port);

			std::string get_hostname();
			size_t get_port();

			// start a persistent incoming socket
			void up();
			void dn();

			// send to a persistent endpoint
			size_t send(std::string message, std::string hostname, size_t port);
			std::string read(size_t len, std::string hostname, size_t port);

		private:
			std::string hostname;
			size_t port;
	};
}

#endif
