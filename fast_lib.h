#include <sys/socket.h>

#ifndef FAST_DEFAULT_MEMPOOL_BUFFERS
#define FAST_DEFAULT_MEMPOOL_BUFFERS   8192 * 4
#endif
#ifndef FAST_DEFAULT_MEMPOOL_CACHE_SIZE
#define FAST_DEFAULT_MEMPOOL_CACHE_SIZE  256
#endif
#ifndef FAST_DEFAULT_MBUF_DATA_SIZE
#define FAST_DEFAULT_MBUF_DATA_SIZE  RTE_MBUF_DEFAULT_BUF_SIZE
#endif

struct fast_mbuf_array {
	struct rte_mbuf *array[512];
	uint32_t n_mbufs;
	uint64_t curr;
	uint64_t drop;
	
};

struct fast_socket{
	uint32_t ip;
	uint16_t id;
	uint64_t stat;
	uint16_t tx_batch_size;
	uint16_t rx_batch_size;
	struct rte_mempool *mem_pool[8];
	struct fast_mbuf_array *array[8];
	struct fast_mbuf_array *recv_buf[8];
	uint8_t *src_mac_be[8];
	uint8_t *dest_mac_be[8];
	uint32_t src_ip[8];
};

int fast_lib_init(void);

struct fast_socket * create_fast_socket(void);

int fast_bind(struct fast_socket *socket,const char *addr);

int fast_sendto(struct fast_socket * socket,const void *buf,size_t len,int flags,const struct sockaddr *dest_addr, socklen_t addrlen, uint16_t port);

int fast_sendmsg(struct fast_socket * socket,const struct msghdr *msg,int flags, uint16_t port);

int fast_recvfrom(struct fast_socket * socket, void *buf, size_t len, int flags, struct sockaddr *src_addr, socklen_t *addrlen,uint16_t port);
