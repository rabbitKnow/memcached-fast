#include <sys/socket.h>


struct fast_socket{
	uint32_t ip;
	struct rte_ring *send_ring;
	struct rte_ring *recv_ring;
	struct rte_mempool *mem_pool;
	uint16_t id;
};

int fast_lib_init(void);

struct fast_socket * create_fast_socket(void);

int fast_bind(struct fast_socket *socket,const char *addr);

int fast_sendto(struct fast_socket * socket,const void *buf,size_t len,int flags,const struct sockaddr *dest_addr, socklen_t addrlen);

int fast_sendmsg(struct fast_socket * socket,const struct msghdr *msg,int flags);

int fast_recvfrom(struct fast_socket * socket, void *buf, size_t len, int flags, struct sockaddr *src_addr, socklen_t *addrlen);
