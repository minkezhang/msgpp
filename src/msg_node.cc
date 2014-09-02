#include "src/msg_node.h"

entangle::MessageNode::MessageNode(std::string hostname, size_t port) : hostname(hostname), port(port) {}

std::string entangle::MessageNode::get_hostname() { return(this->hostname); }
size_t entangle::MessageNode::get_port() { return(this->port); }
