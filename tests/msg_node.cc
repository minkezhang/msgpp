#include <csignal>
#include <memory>
#include <thread>
#include <unistd.h>

#include "libs/catch/catch.hpp"
#include "libs/exceptionpp/exception.h"

#include "src/msg_node.h"

TEST_CASE("msgpp|msg_node") {
	auto n = std::shared_ptr<msgpp::MessageNode> (new msgpp::MessageNode(8080));
	auto m = std::shared_ptr<msgpp::MessageNode> (new msgpp::MessageNode(8081));
	auto o = std::shared_ptr<msgpp::MessageNode> (new msgpp::MessageNode(8082));

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

TEST_CASE("msgpp|msg_node-conn") {
	auto server = std::shared_ptr<msgpp::MessageNode> (new msgpp::MessageNode(8080, msgpp::MessageNode::ipv6));
	auto client = std::shared_ptr<msgpp::MessageNode> (new msgpp::MessageNode(8081));

	REQUIRE_THROWS_AS(client->pull("", 0), exceptionpp::RuntimeError);
	REQUIRE_THROWS_AS(client->pull("a", 1), exceptionpp::RuntimeError);

	auto t = std::thread(&msgpp::MessageNode::up, &*server);

	// check that IPv6 works
	REQUIRE_NOTHROW(client->push("test", "::1", server->get_port()));
	sleep(1);
	REQUIRE(server->pull("", 0).compare("test") == 0);
	raise(SIGINT);
	t.join();
}
