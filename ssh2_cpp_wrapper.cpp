#include "ssh2_cpp_wrapper.hpp"
#include "libssh2_config.h"
#include <libssh2.h>
#include <fstream>

file_transfer_session::~file_transfer_session() {
	libssh2_channel_free(channel);
}


ssh2_conn::ssh2_conn(const char *const address) {
	WSAStartup(MAKEWORD(2, 0), &wsadata);
	auto hostaddr = inet_addr(address);
	libssh2_init(0);
	sock = socket(AF_INET, SOCK_STREAM, 0);
	socket_in.sin_family = AF_INET;
	socket_in.sin_port = htons(22);
	socket_in.sin_addr.s_addr = hostaddr;
}

ssh2_conn::~ssh2_conn() {
	disconnect();
	closesocket(sock);
	libssh2_exit();
}
int ssh2_conn::get_last_error_num() {
	return libssh2_session_last_errno(session);
}
char *ssh2_conn::get_last_error_message() {
	char *errorMsg;
	libssh2_session_last_error(session, &errorMsg, nullptr, 0);
	return errorMsg;
}

int ssh2_conn::connect(const char *username, const char *password) {
	int err = ::connect(sock, reinterpret_cast<struct sockaddr *>(&socket_in),
						sizeof(struct sockaddr_in));
	if (err)
		return err;

	session = libssh2_session_init();
	libssh2_session_handshake(session, sock);
	fingerprint = libssh2_hostkey_hash(session, LIBSSH2_HOSTKEY_HASH_SHA1);
	return libssh2_userauth_password(session, username, password);
}
int ssh2_conn::disconnect() {
	auto res =
		libssh2_session_disconnect(session, "Normal SSH Session Shutdown");
	if (res)
		return res;
	libssh2_session_free(session);
	return res;
}
int ssh2_conn::lookup_file(file_transfer_session &fts, const char *filePath) {
	libssh2_struct_stat fileinfo;
	fts.channel = libssh2_scp_recv2(session, filePath, &fileinfo);
	fts.file_size = fileinfo.st_size;
	if (!fts.channel)
		return -1;
	return 0;
}

int64_t ssh2_conn::receive_file(file_transfer_session &fts, char *dest) {
	auto res = libssh2_channel_read(fts.channel, dest,
									static_cast<uint64_t>(fts.file_size));
	return res;
}

int64_t ssh2_conn::receive_file(const char *src_file, const char *dst_file) {
	file_transfer_session fts{};
	lookup_file(fts, src_file);
	if (!fts.channel)
		return -1;

	std::fstream out_file;
	out_file.open(dst_file, std::ios_base::out | std::ios_base::binary
								| std::ios_base::trunc);
	if (out_file.is_open())
		return -1;

	int64_t rcv_size = 0;
	while (rcv_size < fts.file_size) {
		constexpr auto buf_size = 1024;
		char		   mem[buf_size];
		int64_t		   amount = buf_size;

		if ((fts.file_size - rcv_size) < amount) {
			amount = (fts.file_size - rcv_size);
		}

		auto rc = libssh2_channel_read(fts.channel, mem,
									   static_cast<uint64_t>(amount));
		if (rc > 0) {
			out_file.write(mem, rc);
		} else if (rc < 0) {
			return -1;
		}
		rcv_size += rc;
	}
	return rcv_size;
}
