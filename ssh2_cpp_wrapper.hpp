#ifndef SSH2_CPP_WRAPPER_HPP
#define SSH2_CPP_WRAPPER_HPP
#include <cstdint>
#include <WinSock2.h>


struct _LIBSSH2_CHANNEL;
struct _LIBSSH2_SESSION;

struct file_transfer_session {

	file_transfer_session() = default;
	file_transfer_session(const file_transfer_session &) = delete;
	file_transfer_session(file_transfer_session &&) = default;
	_LIBSSH2_CHANNEL *channel;
	int64_t			  file_size;
	unsigned short	  file_mode;
	~file_transfer_session();
};


class ssh2_conn {

	WSADATA			   wsadata;
	SOCKET			   sock;
	struct sockaddr_in socket_in;
	// unsigned long	   hostaddr;
	_LIBSSH2_SESSION *session;
	const char *	  fingerprint;


public:
	ssh2_conn(const char *const address);

	~ssh2_conn();

	int	  get_last_error_num();
	char *get_last_error_message();

	int connect(const char *username, const char *password);
	int disconnect();
	// file_transfer_session struct should be passed by referenece, since
	// copying the LIBSSH2_CHANNEL ptr spoils the data
	int lookup_file(file_transfer_session &fts, const char *remote_file_path);
	int64_t receive_file_to_buffer(file_transfer_session &fts,
								   char *				  out_buffer);
	int64_t receive_file(const char *remote_file, const char *local_file);

	int propose_file(file_transfer_session &fts, const char *remote_file_path);
	int64_t send_buffer_to_file(const char *		   local_file,
								file_transfer_session &fts);
	int64_t send_file(const char *local_file, const char *remote_file);

	int exec_cmd(const char *cmd, char *res_buffer, size_t res_buffer_size);
};

#endif // SSH2_CPP_WRAPPER_HPP
