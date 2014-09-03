#include <csignal>
#include <memory>
#include <thread>
#include <unistd.h>

#include <sstream>
#include <iostream>

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
	auto client = std::shared_ptr<msgpp::MessageNode> (new msgpp::MessageNode(8081, 0, 5));

	REQUIRE_THROWS_AS(client->pull(""), exceptionpp::RuntimeError);
	REQUIRE_THROWS_AS(client->pull("a"), exceptionpp::RuntimeError);

	REQUIRE_THROWS_AS(client->push("test", "::1", server->get_port()), exceptionpp::RuntimeError);

	auto t = std::thread(&msgpp::MessageNode::up, &*server);

	// check that IPv6 works
	REQUIRE(client->push("test", "::1", server->get_port()) == 4);
	REQUIRE(client->push("abcdef", "127.0.0.1", server->get_port()) == 6);
	REQUIRE(client->push("long long string", "localhost", server->get_port()) == 16);

	REQUIRE_NOTHROW(client->push("foo", "localhost", server->get_port()));

	raise(SIGINT);
	t.join();

	REQUIRE(server->pull().compare("test") == 0);
	REQUIRE(server->pull().compare("abcdef") == 0);
	REQUIRE(server->pull().compare("long long string") == 0);

	REQUIRE_THROWS_AS(server->pull("fake"), exceptionpp::RuntimeError);
	REQUIRE(server->pull("localhost").compare("foo") == 0);

	t = std::thread(&msgpp::MessageNode::up, &*server);

	std::cout << "BEGIN THREADING" << std::endl;

	size_t n_attempts = 10;

	std::vector<std::thread> threads;
	for(size_t i = 0; i < n_attempts; ++i) {
		std::cout << "threading: " << (i + 1) << std::endl;
		std::stringstream buf;
		buf << "foo_" << (i + 1);
		threads.push_back(std::thread(&msgpp::MessageNode::push, &*client, buf.str(), "localhost", server->get_port()));
	}

	for(size_t i = 0; i < n_attempts; ++i) {
		std::cout << "joining: " << (i + 1) << std::endl;
		threads.at(i).join();
	}

	raise(SIGINT);
	t.join();

	REQUIRE(server->query() == n_attempts);

	for(size_t i = 0; i < n_attempts; ++i) {
		std::cout << i << std::endl;
		REQUIRE(server->pull().compare("foo") == 0);
	}
}
