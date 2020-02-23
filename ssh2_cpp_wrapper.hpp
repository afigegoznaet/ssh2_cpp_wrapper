#ifndef SSH2_CPP_WRAPPER_HPP
#define SSH2_CPP_WRAPPER_HPP
#include <cstdint>
#include <WinSock2.h>

struct LIBSSH2_CHANNEL;
struct LIBSSH2_SESSION;

struct file_transfer_session {
	file_transfer_session() = default;
	file_transfer_session(const file_transfer_session &) = delete;
	file_transfer_session(file_transfer_session &&) = default;
	LIBSSH2_CHANNEL *channel;
	int64_t			 file_size;
	~file_transfer_session();
};

class ssh2_conn {
	WSADATA			   wsadata;
	SOCKET			   sock;
	struct sockaddr_in socket_in;
	// unsigned long	   hostaddr;
	LIBSSH2_SESSION *session;
	const char *	 fingerprint;


public:
	ssh2_conn(const char *const address);

	~ssh2_conn();

	int	  get_last_error_num();
	char *get_last_error_message();

	int connect(const char *username, const char *password);
	int disconnect();
	// file_transfer_session struct should be passed by referenece, since
	// copying the LIBSSH2_CHANNEL ptr spoils the data
	int		lookup_file(file_transfer_session &fts, const char *filePath);
	int64_t receive_file(file_transfer_session &fts, char *dest);
	int64_t receive_file(const char *src_file, const char *dst_file);
};


#endif // SSH2_CPP_WRAPPER_HPP
