// RUN: nodecpp-checker %s | FileCheck %s -implicit-check-not="{{warning|error}}:"

#include <safe_ptr.h>
#include <server.h>

using namespace nodecpp::safememory;


struct MyServer2 : public NodeBase {
	
	owning_ptr<nodecpp::SrvMember> server;

	void good() {
		//is ok because 'this' is derived from NodeBase
		server->onEvent([this](naked_ptr<nodecpp::Socket> sock) {}); 

		// but creating instances is an error

		owning_ptr<MyServer2> s = make_owning<MyServer2>();
// CHECK: :[[@LINE-1]]:29: error: (node-dot-cpp)
	}

};



