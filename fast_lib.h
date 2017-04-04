#include <sys/socket.h>


struct fast_socket{
	uint32_t ip;
	struct rte_ring *send_ring;
	struct rte_ring *recv_ring;
	struct rte_mempool *mem_pool;
};

int fast_lib_init(void);

struct fast_socket * create_fast_socket(void);

int fast_bind(struct fast_socket *socket,const char *addr);

int fast_sendto(struct fast_socket * socket,const void *buf,size_t len,int flags,uint32_t dest_addr);

int fast_sendmsg(struct fast_socket * socket,const struct msghdr *msg,int flags);

int fast_recvfrom(struct fast_socket * socket, void *buf, size_t len, int flags,uint32_t *src_addr);
