#include <iostream>

#include <WinSock2.h>
#include <assert.h>
#include <fstream>
#include <sys/types.h>
#include <fcntl.h>
#include <errno.h>
#include <stdio.h>
#include <ctype.h>
#include <vector>
#include "ssh2_cpp_wrapper.hpp"


int main() {
	constexpr auto address = "192.168.0.101";
	constexpr auto username = "roman";
	constexpr auto password = "woodoo";
	constexpr auto scppath = "/tmp/test";
	ssh2_conn	   s2con(address);
	s2con.connect(username, password);
	file_transfer_session fts;
	s2con.lookup_file(fts, scppath);
	std::vector<char> vec(static_cast<size_t>(fts.file_size + 1));
	std::cout << vec.size() << '\n';
	s2con.receive_file(fts, vec.data());
	s2con.receive_file(scppath, "test.txt");
	std::cout << vec.data() << '\n';
	return 0;
}
