#include <cstring>
#include <errno.h>
#include <fcntl.h>
#include <netdb.h>
#include <netinet/in.h>
#include <signal.h>
#include <sstream>
#include <string>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>

#include <iostream>

#include "libs/exceptionpp/exception.h"

#include "src/msg_node.h"

msgpp::Message::Message(std::string ip, std::string hostname, size_t port, std::string message) : ip(ip), hostname(hostname), port(port), message(message) {}

std::string msgpp::Message::get_ip() { return(this->ip); }
std::string msgpp::Message::get_hostname() { return(this->hostname); }
size_t msgpp::Message::get_port() { return(this->port); }
std::string msgpp::Message::get_message() { return(this->message); }

std::vector<std::shared_ptr<msgpp::MessageNode>> msgpp::MessageNode::instances;
std::mutex msgpp::MessageNode::l;

msgpp::MessageNode::MessageNode(size_t port, uint8_t protocol, size_t timeout) : protocol(protocol), port(port), timeout(timeout) {
	this->flag = std::shared_ptr<std::atomic<bool>> (new std::atomic<bool> (0));
}

uint8_t msgpp::MessageNode::get_protocol() { return(this->protocol); }
size_t msgpp::MessageNode::get_port() { return(this->port); }
size_t msgpp::MessageNode::get_timeout() { return(this->timeout); }
void msgpp::MessageNode::set_timeout(size_t timeout) { this->timeout = timeout; }

void msgpp::MessageNode::up() {
	{
		std::lock_guard<std::mutex> lock(msgpp::MessageNode::l);
		if(*flag == 1) {
			return;
		}
		if(msgpp::MessageNode::instances.size() == 0) {
			signal(SIGINT, msgpp::MessageNode::term);
		}
		msgpp::MessageNode::instances.push_back(this->shared_from_this());
	}
	*flag = 1;

	std::stringstream port;
	port << this->port;

	int server_sock, status;
	struct addrinfo info;
	struct addrinfo *list;
	memset(&info, 0, sizeof(info));

	if(this->protocol & msgpp::MessageNode::ipv6) {
		info.ai_family = AF_INET6;
	} else if(this->protocol & msgpp::MessageNode::ipv4) {
		info.ai_family = AF_INET;
	} else {
		info.ai_family = AF_UNSPEC;
	}
	info.ai_socktype = SOCK_STREAM;
	info.ai_flags = AI_PASSIVE;

	status = getaddrinfo(NULL, port.str().c_str(), &info, &list);
	if(list == NULL) {
		throw(exceptionpp::RuntimeError("msgpp::MessageNode::up", "cannot find address"));
	}
	server_sock = socket(list->ai_family, list->ai_socktype, list->ai_protocol);
	if(server_sock == -1) {
		throw(exceptionpp::RuntimeError("msgpp::MessageNode::up", "cannot open socket"));
	}
	int yes = 1;
	status = setsockopt(server_sock, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int));
	status = bind(server_sock, list->ai_addr, list->ai_addrlen);
	if(status == -1) {
		throw(exceptionpp::RuntimeError("msgpp::MessageNode::up", "cannot bind server-side socket"));
	}
	status = listen(server_sock, 100);
	if(status == -1) {
		throw(exceptionpp::RuntimeError("msgpp::MessageNode::up", "cannot listen on server-side socket"));
	}

	// use sockaddr_storage for protocol-agnostic IP storage
	//	cf. http://bit.ly/1ukHOQ8
	struct sockaddr_storage client_addr;
	socklen_t client_size;

	// set as non-blocking
	//	cf. http://bit.ly/1tse7i3
	fcntl(server_sock, F_SETFL, O_NONBLOCK);

	int client_sock;
	while(*(this->flag)) {
		client_sock = accept(server_sock, (sockaddr *) &client_addr, &client_size);
		if(client_sock == -1) {
			sleep(1);
		} else {
			fcntl(client_sock, F_SETFL, O_NONBLOCK);
			std::stringstream len_buf;
			std::stringstream msg_buf;
			bool is_done = 0;
			size_t size = 0;
			while(!is_done) {
				char tmp_buf[1];
				int n_bytes = 0;
				for(size_t i = 0; i < this->get_timeout(); ++i) {
					n_bytes = recv(client_sock, &tmp_buf, 1, 0);
					if(n_bytes == -1) {
						sleep(1);
					} else {
						break;
					}
				}
				std::string tmp = std::string(tmp_buf);
				if(n_bytes == 0 || n_bytes == -1) {
					// client closed unexpectedly
					// as the message queue is atomically set (i.e., no half-assed data), we will roll back changes and not touch the queue
					break;
				}
				if(size == 0) {
					size_t pos = tmp.find(':');
					if(pos == std::string::npos) {
						pos = tmp.size();
					}
					len_buf << tmp.substr(0, pos);
					if(pos != tmp.size()) {
						size = std::stoll(len_buf.str());
						msg_buf << tmp.substr(pos + 1);
					}
				} else {
					msg_buf << tmp_buf;
					is_done = (msg_buf.str().size() >= size);
				}
			}
			if(is_done && (msg_buf.str().size() == size)) {
				std::cout << "buffer: " << msg_buf.str() << std::endl;

				char host[NI_MAXHOST];
				char port[NI_MAXSERV];
				int rc = getnameinfo((struct sockaddr *) &client_addr, client_size, host, NI_MAXHOST, port, NI_MAXSERV, NI_NUMERICHOST | NI_NUMERICSERV);
				{
					std::lock_guard<std::mutex> lock(this->messages_l);
					if (rc == 0) {
						this->messages.push_back(msgpp::Message("", host, std::stoll(std::string(port)), msg_buf.str()));
					} else {
						this->messages.push_back(msgpp::Message("", "", 0, msg_buf.str()));
					}
				}
				std::cout << "done queuing" << std::endl;
				std::cout << "messages length (UP): " << this->messages.size() << std::endl;
			}
			close(client_sock);
		}
	}

	freeaddrinfo(list);
	close(server_sock);

	return;
}

void msgpp::MessageNode::dn() {
	*(this->flag) = 0;
}

size_t msgpp::MessageNode::push(std::string message, std::string hostname, size_t port) {
	std::cout << "in push" << std::endl;
	std::stringstream port_buf, msg_buf;
	port_buf << port;
	msg_buf << message.length() << ":" << message;

	int client_sock, status;
	struct addrinfo info;
	struct addrinfo *list;
	memset(&info, 0, sizeof(info));

	info.ai_family = AF_UNSPEC;
	info.ai_socktype = SOCK_STREAM;

	status = getaddrinfo(hostname.c_str(), port_buf.str().c_str(), &info, &list);
	client_sock = socket(list->ai_family, list->ai_socktype, list->ai_protocol);
	if(client_sock == -1) {
		throw(exceptionpp::RuntimeError("msgpp::MessageNode::push", "cannot open socket"));
	}

	int yes = 1;
	status = setsockopt(client_sock, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int));
	fcntl(client_sock, F_SETFL, O_NONBLOCK);

	for(size_t i = 0; i < this->get_timeout(); ++i) {
		status = connect(client_sock, list->ai_addr, list->ai_addrlen);
		if(status != -1) {
			std::cout << "connected!" << std::endl;
			break;
		}
		std::cout << "cannot connect" << std::endl;
		sleep(1);
	}

	if(status == -1) {
		throw(exceptionpp::RuntimeError("msgpp::MessageNode::push", "cannot connect to destination"));
	}

	int result = send(client_sock, msg_buf.str().c_str(), msg_buf.str().length(), 0);
	if(result == -1) {
		if(errno == EWOULDBLOCK) {}
		throw(exceptionpp::RuntimeError("msgpp::MessageNode::send", "could not send data"));
	} else if((size_t) result != msg_buf.str().length()) {
		throw(exceptionpp::RuntimeError("msgpp::MessageNode::send", "invalid data return size"));
	}

	freeaddrinfo(list);
	close(client_sock);

	std::cout << "sent!" << std::endl;
	return(result);
}

std::string msgpp::MessageNode::pull(std::string hostname, size_t port) {
	std::lock_guard<std::mutex> lock(this->messages_l);

	std::cout << "message length (PULL): " << this->messages.size() << std::endl;

	size_t target = 0;
	bool is_found = 0;
	for(size_t i = 0; i < this->timeout; ++i) {
		if(!this->messages.empty()) {
			for(size_t i = 0; i < this->messages.size(); ++i) {
				bool match_h = (this->messages.at(i).get_hostname().compare("") == 0) || (this->messages.at(i).get_ip().compare("") == 0) || (hostname.compare("") == 0) || (hostname.compare(this->messages.at(i).get_hostname()) == 0) || (hostname.compare(this->messages.at(i).get_ip()) == 0);
				bool match_p = (this->messages.at(i).get_port() == 0) || (port == 0) || (port == this->messages.at(i).get_port());
				if(match_h && match_p) {
					target = i;
					is_found = 1;
					break;
				}
			}
		}
		if(is_found) {
			break;
		}
		sleep(1);
	}

	if(!is_found) {
		throw(exceptionpp::RuntimeError("msgpp::MessageNode::pull", "message does not exist"));
	}

	std::string message = this->messages.at(target).get_message();
	this->messages.erase(this->messages.begin() + target);
	return(message);
}

void msgpp::MessageNode::term(int p) {
	std::lock_guard<std::mutex> lock(msgpp::MessageNode::l);
	for(std::vector<std::shared_ptr<msgpp::MessageNode>>::iterator it = msgpp::MessageNode::instances.begin(); it != msgpp::MessageNode::instances.end(); ++it) {
		(*it)->dn();
	}
	msgpp::MessageNode::instances.clear();
}
