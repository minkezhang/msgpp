#include <csignal>
#include <memory>
#include <thread>
#include <unistd.h>

#include "libs/catch/catch.hpp"
#include "src/msg_node.h"

TEST_CASE("msgpp|msg_node") {
	auto n = std::shared_ptr<msgpp::MessageNode> (new msgpp::MessageNode("", 0));
	auto m = std::shared_ptr<msgpp::MessageNode> (new msgpp::MessageNode("", 0));
	auto o = std::shared_ptr<msgpp::MessageNode> (new msgpp::MessageNode("", 0));

	// start up multiple message nodes
	std::vector<std::thread> t;
	t.push_back(std::thread(&msgpp::MessageNode::up, &*n));
	t.push_back(std::thread(&msgpp::MessageNode::up, &*m));

	sleep(1);

	// send signal to kill nodes
	raise(SIGINT);

	for(std::vector<std::thread>::iterator it = t.begin(); it != t.end(); ++it) {
		it->join();
	}

	// ensure we can do this repeatedly
	t.clear();
	t.push_back(std::thread(&msgpp::MessageNode::up, &*o));

	sleep(1);
	raise(SIGINT);

	t.at(0).join();
}
