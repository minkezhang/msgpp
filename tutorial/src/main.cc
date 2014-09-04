#include <iostream>
#include <memory>
#include <random>
#include <csignal>
#include <sstream>
#include <thread>

#include "libs/msgpp/msg_node.h"

int main() {
	auto s = std::shared_ptr<msgpp::MessageNode> (new msgpp::MessageNode(5555));
	auto c = std::shared_ptr<msgpp::MessageNode> (new msgpp::MessageNode(5556));

	auto ts = std::thread(&msgpp::MessageNode::up, &*s);
	auto tc = std::thread(&msgpp::MessageNode::up, &*c);

	std::cout << "press ^C to stop servers prematurely" << std::endl;

	size_t n_packets = 10;
	for(size_t i = 0; i < n_packets; i++) {
		std::stringstream msg_s, msg_c;
		msg_s << "packet " << (i + 1) << " / " << n_packets << ": message to node S (" << rand() << ")";
		msg_c << "packet " << (i + 1) << " / " << n_packets << ": message to node C (" << rand() << ")";

		s->push(msg_c.str(), "localhost", c->get_port(), true);
		c->push(msg_s.str(), "localhost", s->get_port(), true);
	}

	// wait until at least one message is in both queues
	while(!s->query() || !c->query());

	std::string msg_s, msg_c;
	do {
		msg_s = s->pull("", true);
		msg_c = c->pull("", true);

		std::cout << "s msg: " << msg_s << std::endl;
		std::cout << "c msg: " << msg_c << std::endl;
	} while(msg_s.compare("") || msg_c.compare(""));

	raise(SIGINT);

	ts.join();
	tc.join();

	return(0);
}
