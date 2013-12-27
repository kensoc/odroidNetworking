#ifndef __BACKPORT_NET_IVP6_H
#define __BACKPORT_NET_IVP6_H

#include_next <net/ipv6.h>

static inline int ipv6_addr_v4mapped(const struct in6_addr *a)
{
	        return ((a->s6_addr32[0] | a->s6_addr32[1] | 
					(a->s6_addr32[2] ^ htonl(0x0000ffff))) == 0);
}

static inline void ipv6_addr_set_v4mapped(const __be32 addr,
		struct in6_addr *v4mapped)
{
	ipv6_addr_set(v4mapped,
			0, 0,
			htonl(0x0000FFFF),
			addr);
}

#endif /* __BACKPORT_NET_IVP6_H */
