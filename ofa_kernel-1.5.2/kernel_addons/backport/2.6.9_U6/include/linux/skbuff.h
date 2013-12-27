#ifndef LINUX_SKBUFF_H_BACKPORT
#define LINUX_SKBUFF_H_BACKPORT

#include_next <linux/skbuff.h>

static inline struct sk_buff *netdev_alloc_skb(struct net_device *dev,
		unsigned int length)
{
	struct sk_buff *skb;
	skb = dev_alloc_skb(length);
	if (likely(skb))
		skb->dev = dev;

	return skb;
}

#define CHECKSUM_PARTIAL CHECKSUM_HW 
#define CHECKSUM_COMPLETE CHECKSUM_HW 

static inline int backport_skb_linearize_to_2_6_17(struct sk_buff *skb)
{
	return skb_linearize(skb, GFP_ATOMIC);
}

#define skb_linearize backport_skb_linearize_to_2_6_17

/**
 *      skb_header_release - release reference to header
 *      @skb: buffer to operate on
 *
 *      Drop a reference to the header part of the buffer.  This is done
 *      by acquiring a payload reference.  You must not read from the header
 *      part of skb->data after this.
 */
static inline void skb_header_release(struct sk_buff *skb)
{
}

static inline u16 __skb_checksum_complete(struct sk_buff *skb)
{
	return csum_fold(skb_checksum(skb, 0, skb->len, skb->csum));
}

#define skb_queue_reverse_walk(queue, skb) \
	for (skb = (queue)->prev; \
		prefetch(skb->prev), (skb != (struct sk_buff *)(queue)); \
		skb = skb->prev)

#define gso_size tso_size

#endif
#ifndef __BACKPORT_LINUX_SKBUFF_H_TO_2_6_21__
#define __BACKPORT_LINUX_SKBUFF_H_TO_2_6_21__

#include_next <linux/skbuff.h>

#define transport_header h.raw
#define network_header nh.raw
#define mac_header mac.raw

static inline void skb_reset_mac_header(struct sk_buff *skb)
{
	skb->mac.raw = skb->data;
}

static inline void skb_reset_network_header(struct sk_buff *skb)
{
	skb->network_header = skb->data;
}

static inline void skb_copy_from_linear_data(const struct sk_buff *skb,
					     void *to,
					     const unsigned int len)
{
	memcpy(to, skb->data, len);
}

static inline void skb_copy_to_linear_data(struct sk_buff *skb,
                                           const void *from,
                                           const unsigned int len)
{
        memcpy(skb->data, from, len);
}


static inline unsigned char *skb_end_pointer(const struct sk_buff *skb)
{
	return skb->end;
}

static inline unsigned char *skb_transport_header(const struct sk_buff *skb)
{
	return skb->transport_header;
}

static inline unsigned char *skb_network_header(const struct sk_buff *skb)
{
	return skb->network_header;
}

static inline void skb_reset_transport_header(struct sk_buff *skb)
{
	skb->transport_header = skb->data;
}

static inline int skb_transport_offset(const struct sk_buff *skb)
{
	return skb_transport_header(skb) - skb->data;
}

static inline int skb_network_offset(const struct sk_buff *skb)
{
	return skb_network_header(skb) - skb->data;
}

static inline void skb_set_mac_header(struct sk_buff *skb, const int offset)
{
	skb->mac_header = skb->data + offset;
}

static inline void skb_set_network_header(struct sk_buff *skb, const int offset)
{
	skb->network_header = skb->data + offset;
}

static inline void skb_set_transport_header(struct sk_buff *skb,
					    const int offset)
{
	skb->transport_header = skb->data + offset;
}


static inline int skb_is_gso(const struct sk_buff *skb)
{
        return skb_shinfo(skb)->tso_size;
}
	
static inline void skb_copy_from_linear_data_offset(const struct sk_buff *skb,
                                                    const int offset, void *to,
                                                    const unsigned int len)
{
        memcpy(to, skb->data + offset, len);
}

static inline void skb_record_rx_queue(struct sk_buff *skb, u16 rx_queue)
{
}

/**
 *	__skb_queue_head_init - initialize non-spinlock portions of sk_buff_head
 *	@list: queue to initialize
 *
 *	This initializes only the list and queue length aspects of
 *	an sk_buff_head object.  This allows to initialize the list
 *	aspects of an sk_buff_head without reinitializing things like
 *	the spinlock.  It can also be used for on-stack sk_buff_head
 *	objects where the spinlock is known to not be used.
 */
static inline void __skb_queue_head_init(struct sk_buff_head *list)
{
	list->prev = list->next = (struct sk_buff *)list;
	list->qlen = 0;
}

static inline void __skb_queue_splice(const struct sk_buff_head *list,
				      struct sk_buff *prev,
				      struct sk_buff *next)
{
	struct sk_buff *first = list->next;
	struct sk_buff *last = list->prev;

	first->prev = prev;
	prev->next = first;

	last->next = next;
	next->prev = last;
}

/**
 *	skb_queue_splice - join two skb lists, this is designed for stacks
 *	@list: the new list to add
 *	@head: the place to add it in the first list
 */
static inline void skb_queue_splice(const struct sk_buff_head *list,
				    struct sk_buff_head *head)
{
	if (!skb_queue_empty(list)) {
		__skb_queue_splice(list, (struct sk_buff *) head, head->next);
		head->qlen += list->qlen;
	}
}

/**
 *	skb_queue_splice - join two skb lists and reinitialise the emptied list
 *	@list: the new list to add
 *	@head: the place to add it in the first list
 *
 *	The list at @list is reinitialised
 */
static inline void skb_queue_splice_init(struct sk_buff_head *list,
					 struct sk_buff_head *head)
{
	if (!skb_queue_empty(list)) {
		__skb_queue_splice(list, (struct sk_buff *) head, head->next);
		head->qlen += list->qlen;
		__skb_queue_head_init(list);
	}
}

/**
 *	skb_queue_splice_tail - join two skb lists, each list being a queue
 *	@list: the new list to add
 *	@head: the place to add it in the first list
 */
static inline void skb_queue_splice_tail(const struct sk_buff_head *list,
					 struct sk_buff_head *head)
{
	if (!skb_queue_empty(list)) {
		__skb_queue_splice(list, head->prev, (struct sk_buff *) head);
		head->qlen += list->qlen;
	}
}

/**
 *	skb_queue_splice_tail - join two skb lists and reinitialise the emptied list
 *	@list: the new list to add
 *	@head: the place to add it in the first list
 *
 *	Each of the lists is a queue.
 *	The list at @list is reinitialised
 */
static inline void skb_queue_splice_tail_init(struct sk_buff_head *list,
					      struct sk_buff_head *head)
{
	if (!skb_queue_empty(list)) {
		__skb_queue_splice(list, head->prev, (struct sk_buff *) head);
		head->qlen += list->qlen;
		__skb_queue_head_init(list);
	}
}

#define skb_queue_walk_safe(queue, skb, tmp)					\
		for (skb = (queue)->next, tmp = skb->next;			\
		     skb != (struct sk_buff *)(queue);				\
		     skb = tmp, tmp = skb->next)

#define skb_unlink(skb, list) skb_unlink(skb)

#endif
