#include <signal.h>
#include <string>

#include "src/msg_node.h"

std::vector<std::shared_ptr<msgpp::MessageNode>> msgpp::MessageNode::instances;
std::mutex msgpp::MessageNode::l;

msgpp::MessageNode::MessageNode(std::string hostname, size_t port) : hostname(hostname), port(port) {
	this->flag = std::shared_ptr<std::atomic<bool>> (new std::atomic<bool> (0));
}

std::string msgpp::MessageNode::get_hostname() { return(this->hostname); }
size_t msgpp::MessageNode::get_port() { return(this->port); }

void msgpp::MessageNode::up() {
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

size_t send(std::string message, std::string hostname, size_t port) { return(0); }
std::string recv(size_t len, std::string hostname, size_t port) { return(""); }

void msgpp::MessageNode::term(int p) {
	std::lock_guard<std::mutex> lock(msgpp::MessageNode::l);
	for(std::vector<std::shared_ptr<msgpp::MessageNode>>::iterator it = msgpp::MessageNode::instances.begin(); it != msgpp::MessageNode::instances.end(); ++it) {
		(*it)->dn();
	}
	msgpp::MessageNode::instances.clear();
}
