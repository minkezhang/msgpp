#include <cstring>
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

msgpp::Message::Message(std::string hostname, size_t port, std::string message) : hostname(hostname), port(port), message(message) {}

std::string msgpp::Message::get_hostname() { return(this->hostname); }
size_t msgpp::Message::get_port() { return(this->port); }
std::string msgpp::Message::get_message() { return(this->message); }

std::vector<std::shared_ptr<msgpp::MessageNode>> msgpp::MessageNode::instances;
std::mutex msgpp::MessageNode::l;

msgpp::MessageNode::MessageNode(std::string hostname, size_t port) : hostname(hostname), port(port) {
	this->flag = std::shared_ptr<std::atomic<bool>> (new std::atomic<bool> (0));
}

std::string msgpp::MessageNode::get_hostname() { return(this->hostname); }
size_t msgpp::MessageNode::get_port() { return(this->port); }

void msgpp::MessageNode::up() {
	if(*flag == 1) {
		return;
	}
	{
		std::lock_guard<std::mutex> lock(msgpp::MessageNode::l);
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

	info.ai_flags = AI_PASSIVE;
	status = getaddrinfo(NULL, port.str().c_str(), &info, &list);
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

	struct sockaddr_in client_addr;
	socklen_t client_size;

	// set as non-blocking
	//	cf. http://bit.ly/1tse7i3
	fcntl(server_sock, F_SETFL, O_NONBLOCK);

	int client_sock;
	while(*(this->flag)) {
		do {
			client_sock = accept(server_sock, (struct sockaddr *) &client_addr, &client_size);
			sleep(1);
		} while(client_sock == -1);
		std::cout << "accepted client: " << client_sock << std::endl;
		close(client_sock);
	}

	freeaddrinfo(list);
	close(server_sock);
}

void msgpp::MessageNode::dn() {
	*(this->flag) = 0;
}

size_t msgpp::MessageNode::send(std::string message, std::string hostname, size_t port) { return(0); }

std::string msgpp::MessageNode::recv(std::string hostname, size_t port) {
	std::lock_guard<std::mutex> lock(this->messages_l);
	if(!this->messages.empty()) {
		size_t target = 0;
		bool is_found = 0;
		for(size_t i = 0; i < this->messages.size(); ++i) {
			bool match_h = (hostname.compare("") == 0) || (hostname.compare(this->messages.at(i).get_hostname()) == 0);
			bool match_p = (port == 0) || (port == this->messages.at(i).get_port());
			if(match_h && match_p) {
				target = i;
				is_found = 1;
				break;
			}
		}
		if(!is_found) {
			throw(exceptionpp::RuntimeError("msgpp::MessageNode::recv", "message does not exist"));
		}

		std::string message = this->messages.at(target).get_message();
		this->messages.erase(this->messages.begin() + target);
		return(message);
	}
	else {
		throw(exceptionpp::RuntimeError("msgpp::MessageNode::recv", "message does not exist"));
	}
}

void msgpp::MessageNode::term(int p) {
	std::lock_guard<std::mutex> lock(msgpp::MessageNode::l);
	for(std::vector<std::shared_ptr<msgpp::MessageNode>>::iterator it = msgpp::MessageNode::instances.begin(); it != msgpp::MessageNode::instances.end(); ++it) {
		(*it)->dn();
	}
	msgpp::MessageNode::instances.clear();
}
