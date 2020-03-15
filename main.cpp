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
	constexpr auto username = "user";
	constexpr auto password = "password";
	constexpr auto scppath = "/tmp/test";
	constexpr auto scpSendPath1 = "/tmp/testSend1";
	constexpr auto scpSendPath2 = "/tmp/testSend2";
	ssh2_conn	   s2con(address);
	s2con.connect(username, password);
	/***
	 * Example 0, read remote file to buffer
	 * */
	file_transfer_session fts;
	s2con.lookup_file(fts, scppath);
	std::vector<char> vec(static_cast<size_t>(fts.file_size + 1));
	std::cout << vec.size() << '\n';
	s2con.receive_file_to_buffer(fts, vec.data());
	std::cout << vec.data() << '\n';
	/***
	 * End of example 0
	 * */

	/***
	 * Example 1, read remote file to local file
	 * */
	s2con.receive_file(scppath, "test.txt");
	/***
	 * End of example 1
	 * */

	/***
	 * Example 2, read remote file to buffer
	 * */
	file_transfer_session ftsSend;

	const auto stringToSend = "blahblahblah";
	ftsSend.file_size = strlen(stringToSend);
	s2con.propose_file(ftsSend, scpSendPath1);

	s2con.send_buffer_to_file(stringToSend, ftsSend);
	/***
	 * End of example 2
	 * */

	/***
	 * Example 3, read remote file to local file
	 * */
	s2con.send_file("test.txt", scpSendPath2);
	/***
	 * End of example 3
	 * */

	return 0;
}
