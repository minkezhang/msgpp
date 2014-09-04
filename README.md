msgpp
====

C++ Lightweight Message Passing Interface

Installation
----

```bash
git clone https://github.com/cripplet/msgpp.git
cd giga
git submodule update --init --recursive
make test
```

Updating
----

```bash
git pull
git submodule foreach --recursive git checkout master
git submodule foreach --recursive git pull
```

Example
----

```cpp
// init two nodes with IPv6 support and with timeout of two seconds
auto s = std::shared_ptr<msgpp::MessageNode> (new msgpp::MessageNode(8088, msgpp::MessageNode::ipv6, 2));
auto c = std::shared_ptr<msgpp::MessageNode> (new msgpp::MessageNode(8090, msgpp::MessageNode::ipv6, 2));

// allow 's' to receive messages
auto ts = std::thread(&msgpp::MessageNode::up, &*s);

// send a message "foo" to 's'
c->push("foo", "localhost", s->get_port());

// get the oldest (FIFO) message sent to the 's'
std::string resp = s->pull();

// allow 'c' to also recieve messages
auto tc = std::thread(&msgpp::MessageNode::up, &*t);

// send a response to 'c' from 's' -- the last argument 'true' allows MessageNode::push to return
//	without throwing an exception in the case the message was not delivered
s->push("foo-resp", "localhost", c->get_port(), true);

// get the oldest message sent to 'c'
//	first argument is a hostname / ip filter (i.e. "127.0.0.1" and "localhost"), and
//	the second indicates MessageNode::pull should fail silently and return without throwing
//	an exception in case a message is not found
resp = c->pull("", true);

// shut down all servers within this process
raise(SIGINT);

ts.join();
tc.join();
```

Features
----

* IPv4 and IPv6 support -- send through the internet to other nodes
* simple protocol to emulate -- `LEN:MSG` messages sent through the default `C` socket interface (no need to worry about buffer parsing, etc.)
* atomic messages -- either the message is delivered in whole or dropped by the receiving node

Contact
----

* [github](https://github.com/cripplet/msgpp)
* [gmail](mailto:minke.zhang@gmail.com)
* issues and feature requests should be directed to the the project [issues](https://github.com/cripplet/msgpp/issues) page
