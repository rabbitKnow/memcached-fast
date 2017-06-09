
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <inttypes.h>
#include <stdarg.h>
#include <errno.h>
#include <unistd.h>
#include <termios.h>
#include <sys/queue.h>
#include <sys/socket.h>
#include <arpa/inet.h>

#include <rte_common.h>
#include <rte_memory.h>
#include <rte_memzone.h>
#include <rte_launch.h>
#include <rte_eal.h>
#include <rte_per_lcore.h>
#include <rte_lcore.h>
#include <rte_debug.h>
#include <rte_atomic.h>
#include <rte_branch_prediction.h>
#include <rte_ring.h>
#include <rte_log.h>
#include <rte_mempool.h>
#include <cmdline_rdline.h>
#include <cmdline_parse.h>
#include <cmdline_socket.h>
#include <cmdline.h>
#include <rte_string_fns.h>
#include <rte_mbuf.h>
#include <rte_ethdev.h>
#include <rte_udp.h>
#include <rte_ip.h>
#include <rte_memcpy.h>
#include <rte_byteorder.h>
#include <rte_cycles.h>

#include "fast_lib.h"

static const struct rte_eth_conf port_conf_default = { 
	.rxmode = { .max_rx_pkt_len = ETHER_MAX_LEN }
};


	static int
str_to_unsigned_array(
		const char *s, size_t sbuflen,
		char separator,
		unsigned num_vals,
		unsigned *vals)
{
	char str[sbuflen+1];
	char *splits[num_vals];
	char *endptr = NULL;
	int i, num_splits = 0;

	/* copy s so we don't modify original string */
	snprintf(str, sizeof(str), "%s", s);
	num_splits = rte_strsplit(str, sizeof(str), splits, num_vals, separator);

	errno = 0;
	for (i = 0; i < num_splits; i++) {
		vals[i] = strtoul(splits[i], &endptr, 0);
		if (errno != 0 || *endptr != '\0')
			return -1;
	}

	return num_splits;
}

	static int
str_to_unsigned_vals(
		const char *s,
		size_t sbuflen,
		char separator,
		unsigned num_vals, ...)
{
	unsigned i, vals[num_vals];
	va_list ap;

	num_vals = str_to_unsigned_array(s, sbuflen, separator, num_vals, vals);

	va_start(ap, num_vals);
	for (i = 0; i < num_vals; i++) {
		unsigned *u = va_arg(ap, unsigned *);
		*u = vals[i];
	}
	va_end(ap);
	return num_vals;
}

check_all_ports_link_status(uint8_t port_num, uint32_t port_mask)
{
#define CHECK_INTERVAL 100 /* 100ms */
#define MAX_CHECK_TIME 90 /* 9s (90 * 100ms) in total */
	uint8_t portid, count, all_ports_up, print_flag = 0;
	struct rte_eth_link link;
	uint32_t n_rx_queues, n_tx_queues;

	printf("\nChecking link status");
	fflush(stdout);
	for (count = 0; count <= MAX_CHECK_TIME; count++) {
		all_ports_up = 1;
		for (portid = 0; portid < port_num; portid++) {
			if ((port_mask & (1 << portid)) == 0)
				continue;
			n_rx_queues = 8;
			n_tx_queues = 8;
			if ((n_rx_queues == 0) && (n_tx_queues == 0))
				continue;
			memset(&link, 0, sizeof(link));
			rte_eth_link_get_nowait(portid, &link);
			/* print link status if flag set */
			if (print_flag == 1) {
				if (link.link_status)
					printf("Port %d Link Up - speed %u "
							"Mbps - %s\n", (uint8_t)portid,
							(unsigned)link.link_speed,
							(link.link_duplex == ETH_LINK_FULL_DUPLEX) ?
							("full-duplex") : ("half-duplex\n"));
				else
					printf("Port %d Link Down\n",
							(uint8_t)portid);
				continue;
			}
			/* clear all_ports_up flag if any link down */
			if (link.link_status == 0) {
				all_ports_up = 0;
				break;
			}
		}
		/* after finally printing all link status, get out */
		if (print_flag == 1)
			break;

		if (all_ports_up == 0) {
			printf(".");
			fflush(stdout);
			rte_delay_ms(CHECK_INTERVAL);
		}

		/* set the print_flag if all ports up or timeout */
		if (all_ports_up == 1 || count == (MAX_CHECK_TIME - 1)) {
			print_flag = 1;
			printf("done\n");
		}
	}
}

int fast_lib_init(void){

	int argc=4;
	int ret;
	char **argv=malloc(sizeof(char*)*4);
	//argv[0]="init";

	argv[0]=malloc(20);
	snprintf(argv[0], 20, "init");
	argv[1]=malloc(20);
	snprintf(argv[1], 20, "-n");
	argv[2]=malloc(20);
	snprintf(argv[2], 20, "4");
	argv[3]=malloc(32);
	snprintf(argv[3], 32, "--proc-type=primary");

	ret = rte_eal_init(argc, argv);
	if (ret < 0)
		rte_exit(EXIT_FAILURE, "Cannot init EAL\n");
	return 0;
}


struct fast_socket * create_fast_socket(void){
	char name[32];
	struct fast_socket * socket=malloc(sizeof(struct fast_socket));
	socket->id=0;
	//set batch size
	socket->rx_batch_size=32;
	socket->tx_batch_size=2;

	//init mempool&mbuf array
	int i=0;
	fprintf(stderr,"buf size%d",RTE_MBUF_DEFAULT_BUF_SIZE);
	for(;i<8;i++){

		snprintf(name, sizeof(name), "mbuf_pool_%u", i);
		socket->mem_pool[i]= rte_pktmbuf_pool_create(
				name, FAST_DEFAULT_MEMPOOL_BUFFERS,
				FAST_DEFAULT_MEMPOOL_CACHE_SIZE,
				0, FAST_DEFAULT_MBUF_DATA_SIZE, 0);
		socket->array[i]=malloc(sizeof(struct fast_mbuf_array));
		socket->array[i]->n_mbufs=0;
		socket->array[i]->curr=0;
		socket->array[i]->drop=0;
		socket->recv_buf[i]=malloc(sizeof(struct fast_mbuf_array));
		socket->recv_buf[i]->n_mbufs=0;
		socket->recv_buf[i]->curr=0;
	}
	//init nics
	uint8_t port, queue;
	int ret;
	uint32_t n_rx_queues, n_tx_queues;

	/* Init NIC ports and queues, then start the ports */
	for (port = 0; port < 1; port ++) {

		n_rx_queues = 8;
		n_tx_queues = 8;

		if ((n_rx_queues == 0) && (n_tx_queues == 0)) {
			continue;
		}

		/* Init port */
		printf("Initializing NIC port %u ...\n", (unsigned) port);
		ret = rte_eth_dev_configure(
				port,
				(uint8_t) n_rx_queues,
				(uint8_t) n_tx_queues,
				&port_conf_default);
		if (ret < 0) {
			rte_panic("Cannot init NIC port %u (%d)\n", (unsigned) port, ret);
		}
		rte_eth_promiscuous_enable(port);

		/* Init RX queues */
		queue=0;
		for(;queue<8;queue++){

			printf("Initializing NIC port %u RX queue %u ...\n",
					(unsigned) port,
					(unsigned) queue);
			ret = rte_eth_rx_queue_setup(
					port,
					queue,
					1024,
					0,
					NULL,
					socket->mem_pool[queue]);
			if (ret < 0) {
				rte_panic("Cannot init RX queue %u for port %u (%d)\n",
						(unsigned) queue,
						(unsigned) port,
						ret);
			}

			/* Init TX queues */

			printf("Initializing NIC port %u TX queue %u ...\n",
					(unsigned) port,(unsigned)queue);
			ret = rte_eth_tx_queue_setup(
					port,
					queue,
					1024,
					0,
					NULL);
			if (ret < 0) {
				rte_panic("Cannot init TX queue 0 for port %d (%d)\n",
						port,
						ret);
			}
		}
		/* Start port */
		ret = rte_eth_dev_start(port);
		if (ret < 0) {
			rte_panic("Cannot start port %d (%d)\n", port, ret);
		}

	}
	check_all_ports_link_status(1, (~0x0));

	return socket;
}

int fast_bind(struct fast_socket *socket,const char *addr){	

	if(rte_eal_process_type()==RTE_PROC_PRIMARY){
		return -1;
	}

	const char* p=addr;

	uint32_t ip,ip_a, ip_b, ip_c, ip_d;

	//maybe have error for ip string length
	str_to_unsigned_vals(p, strlen(addr), '.', 4, &ip_a, &ip_b, &ip_c, &ip_d);

	ip = (ip_a << 24) | (ip_b << 16) | (ip_c << 8) | ip_d;

	socket->ip=ip;

	return 0;
}

int fast_sendto(struct fast_socket * socket,const void *buf,size_t len,int flags,const struct sockaddr *dest_addr, socklen_t addrlen, uint16_t queue){
	if(addrlen!=16){
		rte_exit(EXIT_FAILURE, "sendto not support IPV6\n");
		return -2;
	}
	const struct sockaddr_in *dest_addr_in=(struct sockaddr_in *)dest_addr;

	struct rte_mbuf *mbuf= rte_pktmbuf_alloc(socket->mem_pool[queue]);
	if(mbuf==NULL)
		rte_exit(EXIT_FAILURE,"Problem getting membuffer pool\n");
	struct ether_hdr *eth = rte_pktmbuf_mtod(mbuf, struct ether_hdr *); 
	struct ipv4_hdr *ip = (struct ipv4_hdr *)((unsigned char *)eth + sizeof(struct ether_hdr));
	struct udp_hdr *udp = (struct udp_hdr *)((unsigned char *)ip + sizeof(struct ipv4_hdr));
	uint8_t *pkt_data=(uint8_t *)(unsigned char *)udp+sizeof(struct udp_hdr);

	//set for ether
	uint8_t dest_mac_be[6]={0x00, 0x1b, 0x21, 0xa0, 0x36, 0x0d};
	uint8_t src_mac_be[6]={0x00, 0x1b, 0x21, 0xa0, 0x38, 0x6c};
	rte_memcpy((uint8_t *)&eth->d_addr.addr_bytes[0],(uint8_t *)&dest_mac_be,6);
	rte_memcpy((uint8_t *)&eth->s_addr.addr_bytes[0],(uint8_t *)&src_mac_be,6);

	//ether_addr_copy(&saddr, &eth->s_addr);
	eth->ether_type = rte_cpu_to_be_16(ETHER_TYPE_IPv4);

	//set for ipv4_hdr ,wait set other info  
	struct sockaddr_in *in_addr=(struct sockaddr_in *)dest_addr;
	uint32_t dest_addr_be=dest_addr_in->sin_addr.s_addr;
	uint32_t src_addr_be=rte_cpu_to_be_32(socket->ip);

	uint8_t ihl=0x45;
	uint8_t type_of_service=0;
	uint16_t total_length=len+20+8;
	uint16_t total_length_be=rte_cpu_to_be_16(total_length);
	uint8_t time_to_live=0x40;
	uint8_t protocol=0x11;

	uint16_t id_be=rte_cpu_to_be_16(socket->id++);
	uint16_t offset=0x0000;
	uint16_t offset_be=rte_cpu_to_be_16(offset);

	rte_memcpy((uint8_t *)&ip->version_ihl,(uint8_t *)&ihl,1);
	rte_memcpy((uint8_t *)&ip->type_of_service,(uint8_t *)&type_of_service,1);
	rte_memcpy((uint8_t *)&ip->total_length,(uint8_t *)&total_length_be,2);
	rte_memcpy((uint8_t *)&ip->packet_id,(uint8_t *)&id_be,2);
	rte_memcpy((uint8_t *)&ip->fragment_offset,(uint8_t *)&offset_be,2);
	rte_memcpy((uint8_t *)&ip->time_to_live,(uint8_t *)&time_to_live,1);
	rte_memcpy((uint8_t *)&ip->next_proto_id,(uint8_t *)&protocol,1);
	rte_memcpy((uint8_t *)&ip->dst_addr,(uint8_t *)&dest_addr_be,4);
	rte_memcpy((uint8_t *)&ip->src_addr,(uint8_t *)&src_addr_be,4);
	ip->hdr_checksum=0;
	uint16_t checksum_be=rte_ipv4_cksum(ip);
	rte_memcpy((uint8_t *)&ip->hdr_checksum,(uint8_t *)&checksum_be,2);

	//set for udp_hdr
	uint16_t dst_port_be=dest_addr_in->sin_port;
	uint16_t src_port=11211;
	uint16_t src_port_be=rte_cpu_to_be_16(src_port);
	rte_memcpy((uint8_t *)&udp->src_port,(uint8_t *)&src_port_be,2);
	rte_memcpy((uint8_t *)&udp->dst_port,(uint8_t *)&dst_port_be,2);
	udp->dgram_len=rte_cpu_to_be_16(len+8);//include message header
	udp->dgram_cksum=0;
	//copy data before compute udp the checksum
	rte_memcpy(pkt_data,buf,len);
	udp->dgram_cksum=0;
	uint16_t udp_checksum=rte_ipv4_udptcp_cksum(ip,udp);
	udp->dgram_cksum=udp_checksum;

	size_t pkt_size;
	pkt_size=sizeof(struct ether_hdr) + sizeof(struct ipv4_hdr) + sizeof(struct udp_hdr)+len;

	mbuf->data_len=pkt_size;
	mbuf->pkt_len=pkt_size;

	int ret;
	uint32_t n_pkts;
	socket->array[queue]->array[socket->array[queue]->n_mbufs]=mbuf;
	socket->array[queue]->n_mbufs++;
	uint16_t batch_size=socket->tx_batch_size;
	if(socket->array[queue]->n_mbufs==batch_size){	
		n_pkts = rte_eth_tx_burst(0,queue,socket->array[queue]->array,socket->array[queue]->n_mbufs);
		socket->array[queue]->n_mbufs=0;
		//fprintf(stderr, "queue%u,n_pkts= %u\n",queue,n_pkts );
		if(unlikely(n_pkts<batch_size)){

			uint32_t t_mbufs, t_pkts,c_pkts,iter;
			t_mbufs=batch_size;
			t_pkts=n_pkts;
			c_pkts=n_pkts;
			iter=0;
			while(c_pkts<batch_size&&iter<3){
				t_pkts=rte_eth_tx_burst(0,queue,socket->array[queue]->array+c_pkts,t_mbufs-c_pkts);
				c_pkts=c_pkts+t_pkts;
				//iter++;
				//fprintf(stderr, "resend,queue%d,c_pkts%u\n",queue,c_pkts);
			}
			n_pkts=c_pkts;
			//disable	
			if(n_pkts>batch_size){
				uint32_t k;
				//fprintf(stderr,"drop%u",n_pkts);
				for(k = n_pkts; k < batch_size; k ++) {
					rte_pktmbuf_free(socket->array[queue]->array[k]);
				}
				socket->array[queue]->drop=socket->array[queue]->drop+batch_size-n_pkts;

			}
		}
		socket->array[queue]->curr=socket->array[queue]->curr+batch_size;
		//printf state
		/*
		   if(socket->array[queue]->curr>100000000){
		   printf("queue:%d,stat:%llu,drop:%llu\n",queue,socket->array[queue]->curr,socket->array[queue]->drop);
		   socket->array[queue]->curr=0;
		   socket->array[queue]->drop=0;
		   }
		 */
	}

	//wait for check
	flags++;
	return len;
}

int fast_sendmsg(struct fast_socket * socket,const struct msghdr *msg,int flags,uint16_t queue){


	size_t buflen=0;

	int ret;
	void *buf = NULL;
	size_t iov_len=msg->msg_iovlen;
	if(iov_len==1){
		buf=msg->msg_iov->iov_base;
		buflen=msg->msg_iov->iov_len;
	}
	else{
		int iov_i=0;
		for(iov_i=0;iov_i<iov_len;iov_i++){
			buflen+=msg->msg_iov[iov_i].iov_len;	
		}
		buf=malloc(buflen);
		void *cpybuf=buf;
		for(iov_i=0;iov_i<iov_len;iov_i++){
			memcpy(cpybuf,msg->msg_iov[iov_i].iov_base,msg->msg_iov[iov_i].iov_len);
			cpybuf+=msg->msg_iov[iov_i].iov_len;
		}
	}
	struct sockaddr *addr=msg->msg_name;
	ret=fast_sendto(socket,buf,buflen,flags,addr,16,queue);
	return ret;
}

int fast_recvfrom(struct fast_socket * socket, void *buf, size_t len, int flags, struct sockaddr *src_addr, socklen_t *addrlen, uint16_t queue){

	int ret;
	struct rte_mbuf *mbuf;
	for(;;){
		if(socket->recv_buf[queue]->curr<socket->recv_buf[queue]->n_mbufs){
			mbuf=socket->recv_buf[queue]->array[socket->recv_buf[queue]->curr++];
			break;
		}
		else{
			socket->recv_buf[queue]->curr=0;
			socket->recv_buf[queue]->n_mbufs = rte_eth_rx_burst(
					0,
					queue,
					socket->recv_buf[queue]->array,
					socket->rx_batch_size);
			if(socket->recv_buf[queue]->n_mbufs>0){
				//fprintf(stderr,"have\n");
				mbuf=socket->recv_buf[queue]->array[socket->recv_buf[queue]->curr++];
				break;
			}
			else{
				continue;
			}
		}
	}

	struct ether_hdr *eth = rte_pktmbuf_mtod(mbuf, struct ether_hdr *); 
	struct ipv4_hdr *ip = (struct ipv4_hdr *)((unsigned char *)eth + sizeof(struct ether_hdr));
	struct udp_hdr *udp = (struct udp_hdr *)((unsigned char *)ip + sizeof(struct ipv4_hdr));
	uint8_t *pkt_data=(uint8_t *)(unsigned char *)udp+sizeof(struct udp_hdr);

	uint16_t data_len=rte_be_to_cpu_16(udp->dgram_len)-8;
	if(data_len>len)
		return -2;
	//set ip and port from recv packet
	struct sockaddr_in *addr=(struct sockaddr_in *)src_addr;
	addr->sin_addr.s_addr=ip->src_addr;
	addr->sin_port=udp->src_port;
	*addrlen=sizeof(struct sockaddr_in);
	rte_memcpy(buf,pkt_data,data_len);

	rte_pktmbuf_free(mbuf);
	//wait for check
	flags++;
	return data_len;
}
