#include "libssh2_config.h"
#include <libssh2.h>
#include "ssh2_cpp_wrapper.hpp"
#include <fstream>
#include <sys/types.h>

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
int ssh2_conn::lookup_file(file_transfer_session &fts,
						   const char *			  remote_file_path) {
	libssh2_struct_stat fileinfo;
	fts.channel = libssh2_scp_recv2(session, remote_file_path, &fileinfo);
	fts.file_size = fileinfo.st_size;
	if (!fts.channel)
		return -1;
	return 0;
}

int64_t ssh2_conn::receive_file_to_buffer(file_transfer_session &fts,
										  char *				 out_buffer) {
	auto res = libssh2_channel_read(fts.channel, out_buffer,
									static_cast<uint64_t>(fts.file_size));
	return res;
}

int64_t ssh2_conn::receive_file(const char *remote_file,
								const char *local_file) {
	file_transfer_session fts{};
	lookup_file(fts, remote_file);
	if (!fts.channel)
		return -1;

	std::fstream out_file;
	out_file.open(local_file, std::ios_base::out | std::ios_base::binary
								  | std::ios_base::trunc);
	if (!out_file.is_open())
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

int ssh2_conn::propose_file(file_transfer_session &fts,
							const char *		   remote_file_path) {
	// libssh2_struct_stat fileinfo;

	// if (stat(remote_file_path, reinterpret_cast<struct stat *>(&fileinfo)))
	// return -1;

	if (!fts.file_size)
		return -1;

	fts.channel =
		libssh2_scp_send(session, remote_file_path, fts.file_mode & 0777,
						 static_cast<unsigned long>(fts.file_size));

	if (!fts.channel)
		return -1;

	return 0;
}

int64_t ssh2_conn::send_buffer_to_file(const char *			  source_buffer,
									   file_transfer_session &fts) {
	return libssh2_channel_write(fts.channel, source_buffer,
								 static_cast<uint64_t>(fts.file_size));
	fprintf(stderr, "Sending EOF\n");
	libssh2_channel_send_eof(fts.channel);

	fprintf(stderr, "Waiting for EOF\n");
	libssh2_channel_wait_eof(fts.channel);

	fprintf(stderr, "Waiting for channel to close\n");
	libssh2_channel_wait_closed(fts.channel);
}

int64_t ssh2_conn::send_file(const char *local_file, const char *remote_file) {
	libssh2_struct_stat fileinfo;

	if (stat(local_file, reinterpret_cast<struct stat *>(&fileinfo)))
		return -1;

	std::fstream input_file;
	input_file.open(local_file, std::ios_base::in | std::ios_base::binary);
	if (!input_file.is_open())
		return -1;

	file_transfer_session fts;
	fts.file_mode = fileinfo.st_mode;
	fts.file_size = fileinfo.st_size;
	propose_file(fts, remote_file);
	if (!fts.channel)
		return -1;


	int64_t snd_size = 0;
	while (snd_size < fts.file_size) {
		constexpr auto buf_size = 1024;
		char		   mem[buf_size];
		int64_t		   amount = buf_size;

		if ((fts.file_size - snd_size) < amount) {
			amount = (fts.file_size - snd_size);
		}
		input_file.read(mem, amount);

		if (input_file) { // returns false on error
			auto rc = libssh2_channel_write(fts.channel, mem,
											static_cast<uint64_t>(amount));
			snd_size += rc;
		} else {
			break;
		}
	}


	// fprintf(stderr, "Sending EOF\n");
	libssh2_channel_send_eof(fts.channel);

	// fprintf(stderr, "Waiting for EOF\n");
	libssh2_channel_wait_eof(fts.channel);

	// fprintf(stderr, "Waiting for channel to close\n");
	libssh2_channel_wait_closed(fts.channel);

	return snd_size;
}

int ssh2_conn::exec_cmd(const char *cmd, char *res_buffer,
						size_t res_buffer_size) {

	auto channel = libssh2_channel_open_session(session);
	if (channel == nullptr)
		return -1;

	auto rc = libssh2_channel_exec(channel, cmd);
	// fprintf(stderr, "channel exec %d\n", res);
	int bytecount = 0;

	if (0 == rc) {
		rc = 1;
		while (rc > 0) {
			// char buffer[0x4000];
			rc = libssh2_channel_read(channel, res_buffer, res_buffer_size);
		}
	}


	libssh2_channel_close(channel);
	libssh2_channel_get_exit_status(channel);
	libssh2_channel_free(channel);
	return rc;
}
