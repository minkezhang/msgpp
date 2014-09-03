#include <signal.h>
#include <string>

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
	while(*(this->flag));
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
