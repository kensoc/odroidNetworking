/*
 * Copyright (c) 2009,2010 Mellanox Technologies Ltd. All rights reserved.
 * Copyright (c) 2009,2010 System Fabric Works, Inc. All rights reserved.
 *
 * This software is available to you under a choice of one of two
 * licenses.  You may choose to be licensed under the terms of the GNU
 * General Public License (GPL) Version 2, available from the file
 * COPYING in the main directory of this source tree, or the
 * OpenIB.org BSD license below:
 *
 *	   Redistribution and use in source and binary forms, with or
 *	   without modification, are permitted provided that the following
 *	   conditions are met:
 *
 *		- Redistributions of source code must retain the above
 *		  copyright notice, this list of conditions and the following
 *		  disclaimer.
 *
 *		- Redistributions in binary form must reproduce the above
 *		  copyright notice, this list of conditions and the following
 *		  disclaimer in the documentation and/or other materials
 *		  provided with the distribution.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS
 * BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
 * ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

/* qp implementation details */

#include <linux/skbuff.h>
#include <linux/debugfs.h>

#include "rxe.h"
#include "rxe_loc.h"
#include "rxe_debug.h"
#include "rxe_mcast.h"
#include "rxe_queue.h"
#include "rxe_av.h"
#include "rxe_qp.h"
#include "rxe_mmap.h"
#include "rxe_task.h"

char *rxe_qp_state_name[] = {
	[QP_STATE_RESET]	= "RESET",
	[QP_STATE_INIT]		= "INIT",
	[QP_STATE_READY]	= "READY",
	[QP_STATE_DRAIN]	= "DRAIN",
	[QP_STATE_DRAINED]	= "DRAINED",
	[QP_STATE_ERROR]	= "ERROR",
};

static int rxe_qp_chk_cap(struct rxe_dev *rxe, struct ib_qp_cap *cap,
			  int has_srq)
{
	if (cap->max_send_wr > rxe->attr.max_qp_wr) {
		rxe_warn(rxe, "invalid send wr = %d > %d\n",
			cap->max_send_wr, rxe->attr.max_qp_wr);
		goto err1;
	}

	if (cap->max_send_sge > rxe->attr.max_sge) {
		rxe_warn(rxe, "invalid send sge = %d > %d\n",
			cap->max_send_sge, rxe->attr.max_sge);
		goto err1;
	}

	if (!has_srq) {
		if (cap->max_recv_wr > rxe->attr.max_qp_wr) {
			rxe_warn(rxe, "invalid recv wr = %d > %d\n",
				cap->max_recv_wr, rxe->attr.max_qp_wr);
			goto err1;
		}

		if (cap->max_recv_sge > rxe->attr.max_sge) {
			rxe_warn(rxe, "invalid recv sge = %d > %d\n",
				  cap->max_recv_sge, rxe->attr.max_sge);
			goto err1;
		}
	}

	if (cap->max_inline_data > rxe->max_inline_data) {
		rxe_warn(rxe, "invalid max inline data = %d > %d\n",
			cap->max_inline_data, rxe->max_inline_data);
		goto err1;
	}

	return 0;

err1:
	return -EINVAL;
}

int rxe_qp_chk_init(struct rxe_dev *rxe, struct ib_qp_init_attr *init)
{
	struct ib_qp_cap *cap = &init->cap;
	struct rxe_port *port;
	int port_num = init->port_num;

	if (!init->recv_cq || !init->send_cq) {
		rxe_warn(rxe, "missing cq\n");
		goto err1;
	}

	if (rxe_qp_chk_cap(rxe, cap, init->srq != NULL))
		goto err1;

	if (init->qp_type != IB_QPT_XRC && init->xrc_domain) {
		rxe_warn(rxe, "xrc domain for qp type not xrc\n");
		goto err1;
	}

	if (init->qp_type == IB_QPT_SMI || init->qp_type == IB_QPT_GSI) {
		if (port_num < 1 || port_num > rxe->num_ports) {
			rxe_warn(rxe, "invalid port = %d\n", port_num);
			goto err1;
		}

		port = &rxe->port[port_num - 1];

		if (init->qp_type == IB_QPT_SMI && port->qp_smi_index) {
			rxe_warn(rxe, "SMI QP exists for port %d\n", port_num);
			goto err1;
		}

		if (init->qp_type == IB_QPT_GSI && port->qp_gsi_index) {
			rxe_warn(rxe, "GSI QP exists for port %d\n", port_num);
			goto err1;
		}
	}

	return 0;

err1:
	return -EINVAL;
}

static int alloc_rd_atomic_resources(struct rxe_qp *qp, unsigned int n)
{
	qp->resp.res_head = 0;
	qp->resp.res_tail = 0;
	qp->resp.resources = kcalloc(n, sizeof(struct resp_res), GFP_KERNEL);

	if (!qp->resp.resources)
		return -ENOMEM;

	return 0;
}

static void free_rd_atomic_resources(struct rxe_qp *qp)
{
	if (qp->resp.resources) {
		int i;

		for (i = 0; i < qp->attr.max_rd_atomic; i++) {
			struct resp_res *res = &qp->resp.resources[i];
			free_rd_atomic_resource(qp, res);
		}
		kfree(qp->resp.resources);
		qp->resp.resources = NULL;
	}
}

void free_rd_atomic_resource(struct rxe_qp *qp, struct resp_res *res)
{
	struct rxe_dev *rxe = to_rdev(qp->ibqp.device);

	if (res->type == RXE_ATOMIC_MASK) {
		rxe_drop_ref(qp);
		kfree_skb(res->atomic.skb);
		atomic_dec(&rxe->resp_skb_out);
	} else if (res->type == RXE_READ_MASK) {
		if (res->read.mr)
			rxe_drop_ref(res->read.mr);
	}
	res->type = 0;
}

static void cleanup_rd_atomic_resources(struct rxe_qp *qp)
{
	int i;
	struct resp_res *res;

	if (qp->resp.resources) {
		for (i = 0; i < qp->attr.max_rd_atomic; i++) {
			res = &qp->resp.resources[i];
			free_rd_atomic_resource(qp, res);
		}
	}
}

static void rxe_qp_init_misc(struct rxe_dev *rxe, struct rxe_qp *qp,
			     struct ib_qp_init_attr *init)
{
	struct rxe_port *port;
	u32 qpn;

	qp->sq_sig_type		= init->sq_sig_type;
	qp->attr.path_mtu	= 1;
	qp->mtu			= 256;

	qpn			= qp->pelem.index;
	port			= &rxe->port[init->port_num - 1];

	switch (init->qp_type) {
	case IB_QPT_SMI:
		qp->ibqp.qp_num		= 0;
		port->qp_smi_index	= qpn;
		qp->attr.port_num	= init->port_num;
		break;

	case IB_QPT_GSI:
		qp->ibqp.qp_num		= 1;
		port->qp_gsi_index	= qpn;
		qp->attr.port_num	= init->port_num;
		break;

	default:
		qp->ibqp.qp_num		= qpn;
		break;
	}

	INIT_LIST_HEAD(&qp->arbiter_list);
	INIT_LIST_HEAD(&qp->grp_list);
	INIT_LIST_HEAD(&qp->xrc_reg_list);

	skb_queue_head_init(&qp->send_pkts);

	spin_lock_init(&qp->grp_lock);
	spin_lock_init(&qp->state_lock);

	atomic_set(&qp->ssn, 0);
	atomic_set(&qp->req_skb_in, 0);
	atomic_set(&qp->resp_skb_in, 0);
	atomic_set(&qp->req_skb_out, 0);
	atomic_set(&qp->resp_skb_out, 0);
}

static int rxe_qp_init_req(struct rxe_dev *rxe, struct rxe_qp *qp,
			   struct ib_qp_init_attr *init,
			   struct ib_ucontext *context, struct ib_udata *udata)
{
	int err;
	int wqe_size;

	qp->sq.max_wr		= init->cap.max_send_wr;
	qp->sq.max_sge		= init->cap.max_send_sge;
	qp->sq.max_inline	= init->cap.max_inline_data;

	wqe_size = max_t(int, sizeof(struct rxe_send_wqe) +
				qp->sq.max_sge*sizeof(struct ib_sge),
				sizeof(struct rxe_send_wqe) +
					qp->sq.max_inline);

	qp->sq.queue		= rxe_queue_init(rxe,
						 (unsigned int *)&qp->sq.max_wr,
						 wqe_size);
	if (!qp->sq.queue)
		return -ENOMEM;

#ifdef RXE_USER_SEND_QUEUE
	err = do_mmap_info(rxe, udata, sizeof(struct mminfo),
			   context, qp->sq.queue->buf,
			   qp->sq.queue->buf_size, &qp->sq.queue->ip);
#endif

	if (err) {
		vfree(qp->sq.queue->buf);
		kfree(qp->sq.queue);
	}

	qp->req.wqe_index	= producer_index(qp->sq.queue);
	qp->req.state		= QP_STATE_RESET;
	qp->req.opcode		= -1;
	qp->comp.opcode		= -1;

	spin_lock_init(&qp->sq.sq_lock);
	skb_queue_head_init(&qp->req_pkts);

	rxe_init_task(rxe, &qp->req.task, &rxe_fast_req, qp,
		      rxe_requester, "req");
	rxe_init_task(rxe, &qp->comp.task, &rxe_fast_comp, qp,
		      rxe_completer, "comp");

	init_timer(&qp->rnr_nak_timer);
	qp->rnr_nak_timer.function = rnr_nak_timer;
	qp->rnr_nak_timer.data = (unsigned long)qp;

	init_timer(&qp->retrans_timer);
	qp->retrans_timer.function = retransmit_timer;
	qp->retrans_timer.data = (unsigned long)qp;
	qp->qp_timeout_jiffies = 0; /* Can't be set for UD/UC in modify_qp */

	return 0;
}

static int rxe_qp_init_resp(struct rxe_dev *rxe, struct rxe_qp *qp,
			    struct ib_qp_init_attr *init,
			    struct ib_ucontext *context, struct ib_udata *udata)
{
	int err;
	int wqe_size;

	if (!qp->srq) {
		qp->rq.max_wr		= init->cap.max_recv_wr;
		qp->rq.max_sge		= init->cap.max_recv_sge;

		wqe_size = sizeof(struct rxe_recv_wqe) +
			       qp->rq.max_sge*sizeof(struct ib_sge);

		rxe_debug(rxe, "max_wr = %d, max_sge = %d, wqe_size = %d\n",
			qp->rq.max_wr, qp->rq.max_sge, wqe_size);

		qp->rq.queue		= rxe_queue_init(rxe,
						(unsigned int *)&qp->rq.max_wr,
						wqe_size);
		if (!qp->rq.queue)
			return -ENOMEM;

		err = do_mmap_info(rxe, udata, 0, context, qp->rq.queue->buf,
				   qp->rq.queue->buf_size, &qp->rq.queue->ip);
		if (err) {
			vfree(qp->rq.queue->buf);
			kfree(qp->rq.queue);
		}

	}

	spin_lock_init(&qp->rq.producer_lock);
	spin_lock_init(&qp->rq.consumer_lock);

	skb_queue_head_init(&qp->resp_pkts);

	rxe_init_task(rxe, &qp->resp.task, &rxe_fast_resp, qp,
		      rxe_responder, "resp");

	qp->resp.opcode		= OPCODE_NONE;
	qp->resp.msn		= 0;
	qp->resp.state		= QP_STATE_RESET;

	return 0;
}

/* called by the create qp verb */
int rxe_qp_from_init(struct rxe_dev *rxe, struct rxe_qp *qp, struct rxe_pd *pd,
		     struct ib_qp_init_attr *init, struct ib_udata *udata,
		     struct ib_pd *ibpd)
{
	int err;
	struct rxe_cq *rcq = to_rcq(init->recv_cq);
	struct rxe_cq *scq = to_rcq(init->send_cq);
	struct rxe_srq *srq = init->srq ? to_rsrq(init->srq) : NULL;
	struct rxe_xrcd *xrcd = init->xrc_domain ?
				to_rxrcd(init->xrc_domain) : NULL;
	struct ib_ucontext *context = udata ? ibpd->uobject->context : NULL;

	rxe_add_ref(pd);
	rxe_add_ref(rcq);
	rxe_add_ref(scq);
	if (srq)
		rxe_add_ref(srq);
	if (xrcd)
		rxe_add_ref(xrcd);

	qp->pd			= pd;
	qp->rcq			= rcq;
	qp->scq			= scq;
	qp->srq			= srq;
	qp->udata		= udata;
	qp->xrcd		= xrcd;

	if (xrcd)
		rxe_debug(rxe, "creating an ini xrc qp\n");

	rxe_qp_init_misc(rxe, qp, init);

	err = rxe_qp_init_req(rxe, qp, init, context, udata);
	if (err)
		goto err1;

	err = rxe_qp_init_resp(rxe, qp, init, context, udata);
	if (err)
		goto err2;

#ifdef CONFIG_RXE_DEBUGFS
	snprintf(qp->dfs.qpn, sizeof(qp->dfs.qpn), "%06x",
			qp->ibqp.qp_num);

	qp->pelem.dfs_dir = debugfs_create_dir(qp->dfs.qpn,
			rxe->dfs.qp_dir);

	debugfs_create_u32("msn", 0444, qp->pelem.dfs_dir,
			&qp->resp.msn);

	debugfs_create_u32("comp.psn", 0444, qp->pelem.dfs_dir,
			&qp->comp.psn);
	debugfs_create_u32("comp.timeout", 0444, qp->pelem.dfs_dir,
			&qp->comp.timeout);
	debugfs_create_u32("comp.retry_cnt", 0444, qp->pelem.dfs_dir,
			&qp->comp.retry_cnt);
	debugfs_create_u32("comp.rnr_retry", 0444, qp->pelem.dfs_dir,
			&qp->comp.rnr_retry);

	debugfs_create_u32("req.psn", 0444, qp->pelem.dfs_dir,
			&qp->req.psn);
	debugfs_create_u32("req.state", 0666, qp->pelem.dfs_dir,
			&qp->req.state);

	debugfs_create_u32("resp.psn", 0444, qp->pelem.dfs_dir,
			&qp->resp.psn);
	debugfs_create_u32("resp.state", 0666, qp->pelem.dfs_dir,
			(__force unsigned int *)&qp->resp.state);
	debugfs_create_u32("resp.sent_psn_nak", 0444, qp->pelem.dfs_dir,
			&qp->resp.sent_psn_nak);

	debugfs_create_u32("valid", 0444, qp->pelem.dfs_dir,
			&qp->valid);
	debugfs_create_u64("qp_timeout_jiffies", 0444, qp->pelem.dfs_dir,
			&qp->qp_timeout_jiffies);
	debugfs_create_u32("refcount", 0444, qp->pelem.dfs_dir,
			(u32 *)&qp->pelem.ref_cnt.refcount.counter);
#endif

	qp->attr.qp_state = IB_QPS_RESET;
	qp->valid = 1;

	return 0;

err2:
	rxe_queue_cleanup(qp->sq.queue);
err1:
	if (xrcd)
		rxe_drop_ref(xrcd);
	if (srq)
		rxe_drop_ref(srq);
	rxe_drop_ref(scq);
	rxe_drop_ref(rcq);
	rxe_drop_ref(pd);

	return err;
}

void rxe_xrc_rcv_qp_from_init(struct rxe_qp *qp, struct ib_qp_init_attr *init)
{
	struct rxe_xrcd *xrcd = to_rxrcd(init->xrc_domain);

	rxe_add_ref(xrcd);
	qp->xrcd		= xrcd;

	qp->sq_sig_type		= init->sq_sig_type;
	qp->attr.path_mtu	= 1;
	qp->mtu			= 256;
	qp->ibqp.qp_num		= qp->pelem.index;
	qp->ibqp.device		= init->xrc_domain->device;
	qp->ibqp.event_handler	= init->event_handler;
	qp->ibqp.qp_context	= init->qp_context;
	qp->ibqp.qp_type	= init->qp_type;
	qp->ibqp.xrcd		= init->xrc_domain;

	spin_lock_init(&qp->state_lock);

	INIT_LIST_HEAD(&qp->arbiter_list);
	INIT_LIST_HEAD(&qp->xrc_reg_list);

	atomic_set(&qp->req_skb_in, 0);
	atomic_set(&qp->resp_skb_in, 0);
	atomic_set(&qp->req_skb_out, 0);
	atomic_set(&qp->resp_skb_out, 0);

	skb_queue_head_init(&qp->resp_pkts);

	rxe_init_task(to_rdev(qp->ibqp.device), &qp->resp.task,
		      &rxe_fast_resp, qp, rxe_responder, "resp");

	qp->resp.opcode		= OPCODE_NONE;
	qp->resp.msn		= 0;
	qp->resp.state		= QP_STATE_RESET;

	qp->attr.qp_state	= IB_QPS_RESET;
	qp->valid		= 1;
}

/* called by the query qp verb */
int rxe_qp_to_init(struct rxe_qp *qp, struct ib_qp_init_attr *init)
{
	init->event_handler		= qp->ibqp.event_handler;
	init->qp_context		= qp->ibqp.qp_context;
	init->send_cq			= qp->ibqp.send_cq;
	init->recv_cq			= qp->ibqp.recv_cq;
	init->srq			= qp->ibqp.srq;
	init->xrc_domain		= qp->xrcd ? &qp->xrcd->ibxrcd : NULL;

	init->cap.max_send_wr		= qp->sq.max_wr;
	init->cap.max_send_sge		= qp->sq.max_sge;
	init->cap.max_inline_data	= qp->sq.max_inline;

	if (!qp->srq) {
		init->cap.max_recv_wr		= qp->rq.max_wr;
		init->cap.max_recv_sge		= qp->rq.max_sge;
	}

	init->sq_sig_type		= qp->sq_sig_type;

	init->qp_type			= qp->ibqp.qp_type;
	init->port_num			= 1;

	return 0;
}

/* called by the modify qp verb, this routine
   checks all the parameters before making any changes */
int rxe_qp_chk_attr(struct rxe_dev *rxe, struct rxe_qp *qp,
		    struct ib_qp_attr *attr, int mask)
{
	enum ib_qp_state cur_state = (mask & IB_QP_CUR_STATE) ?
					attr->cur_qp_state : qp->attr.qp_state;
	enum ib_qp_state new_state = (mask & IB_QP_STATE) ?
					attr->qp_state : cur_state;

	if (!ib_modify_qp_is_ok(cur_state, new_state, qp_type(qp), mask)) {
		rxe_warn(rxe, "invalid mask or state for qp\n");
		goto err1;
	}

	if (mask & IB_QP_STATE) {
		if (cur_state == IB_QPS_SQD) {
			if (qp->req.state == QP_STATE_DRAIN &&
					new_state != IB_QPS_ERR)
				goto err1;
		}
	}

	if (mask & IB_QP_PORT) {
		if (attr->port_num < 1 || attr->port_num > rxe->num_ports) {
			rxe_warn(rxe, "invalid port %d\n", attr->port_num);
			goto err1;
		}
	}

	if (mask & IB_QP_CAP && rxe_qp_chk_cap(rxe, &attr->cap,
					       qp->srq != NULL))
		goto err1;

	if (mask & IB_QP_AV && rxe_av_chk_attr(rxe, &attr->ah_attr))
		goto err1;

	if (mask & IB_QP_ALT_PATH && rxe_av_chk_attr(rxe, &attr->alt_ah_attr))
		goto err1;

	if (mask & IB_QP_PATH_MTU) {
		struct rxe_port *port = &rxe->port[qp->attr.port_num - 1];
		enum rxe_mtu max_mtu = (enum rxe_mtu __force)port->attr.max_mtu;
		enum rxe_mtu mtu = (enum rxe_mtu __force)attr->path_mtu;

		if (mtu > max_mtu) {
			rxe_debug(rxe, "invalid mtu (%d) > (%d)\n",
				  rxe_mtu_enum_to_int(mtu),
				  rxe_mtu_enum_to_int(max_mtu));
			goto err1;
		}
	}

	if (mask & IB_QP_MAX_QP_RD_ATOMIC) {
		if (attr->max_rd_atomic > rxe->attr.max_qp_rd_atom) {
			rxe_warn(rxe, "invalid max_rd_atomic %d > %d\n",
				 attr->max_rd_atomic,
				 rxe->attr.max_qp_rd_atom);
			goto err1;
		}
	}

	if (mask & IB_QP_TIMEOUT) {
		if (attr->timeout > 31) {
			rxe_warn(rxe, "invalid QP timeout %d > 31\n",
				 attr->timeout);
			goto err1;
		}
	}

	return 0;

err1:
	return -EINVAL;
}

/* move the qp to the reset state */
static void rxe_qp_reset(struct rxe_qp *qp)
{
	/* stop tasks from running */
	rxe_disable_task(&qp->resp.task);

	/* stop request/comp
	   we use qp->sq.queue == 0 to
	   recognize xrc rcv type qp's
	   which have no request logic */
	if (qp->sq.queue) {
		if (qp_type(qp) == IB_QPT_RC)
			rxe_disable_task(&qp->comp.task);
		rxe_disable_task(&qp->req.task);
	}

	/* move qp to the reset state */
	qp->req.state = QP_STATE_RESET;
	qp->resp.state = QP_STATE_RESET;

	/* let state machines reset themselves
	   drain work and packet queues etc. */
	__rxe_do_task(&qp->resp.task);

	if (qp->sq.queue) {
		__rxe_do_task(&qp->comp.task);
		__rxe_do_task(&qp->req.task);
	}

	/* cleanup attributes */
	atomic_set(&qp->ssn, 0);
	qp->req.opcode = -1;
	qp->req.need_retry = 0;
	qp->req.noack_pkts = 0;
	qp->resp.msn = 0;
	qp->resp.opcode = -1;
	qp->resp.drop_msg = 0;
	qp->resp.goto_error = 0;
	qp->resp.sent_psn_nak = 0;

	if (qp->resp.mr) {
		rxe_drop_ref(qp->resp.mr);
		qp->resp.mr = NULL;
	}

	cleanup_rd_atomic_resources(qp);

	/* reenable tasks */
	rxe_enable_task(&qp->resp.task);

	if (qp->sq.queue) {
		if (qp_type(qp) == IB_QPT_RC)
			rxe_enable_task(&qp->comp.task);

		rxe_enable_task(&qp->req.task);
	}
}

/* drain the send queue */
static void rxe_qp_drain(struct rxe_qp *qp)
{
	if (qp->sq.queue) {
		if (qp->req.state != QP_STATE_DRAINED) {
			qp->req.state = QP_STATE_DRAIN;
			if (qp_type(qp) == IB_QPT_RC)
				rxe_run_task(&qp->comp.task, 1);
			else
				__rxe_do_task(&qp->comp.task);
			rxe_run_task(&qp->req.task, 1);
		}
	}
}

/* move the qp to the error state */
void rxe_qp_error(struct rxe_qp *qp)
{
	qp->req.state = QP_STATE_ERROR;
	qp->resp.state = QP_STATE_ERROR;

	/* drain work and packet queues */
	rxe_run_task(&qp->resp.task, 1);

	/* if not xrc rcv qp */
	if (qp->sq.queue) {
		if (qp_type(qp) == IB_QPT_RC)
			rxe_run_task(&qp->comp.task, 1);
		else
			__rxe_do_task(&qp->comp.task);
		rxe_run_task(&qp->req.task, 1);
	}
}

/* called by the modify qp verb */
int rxe_qp_from_attr(struct rxe_qp *qp, struct ib_qp_attr *attr, int mask,
		     struct ib_udata *udata)
{
	int err;
	struct rxe_dev *rxe = to_rdev(qp->ibqp.device);

	/* TODO should handle error by leaving old resources intact */
	if (mask & IB_QP_MAX_QP_RD_ATOMIC) {
		int max_rd_atomic = __roundup_pow_of_two(attr->max_rd_atomic);

		free_rd_atomic_resources(qp);

		err = alloc_rd_atomic_resources(qp, max_rd_atomic);
		if (err)
			return err;

		qp->attr.max_rd_atomic = max_rd_atomic;
		atomic_set(&qp->req.rd_atomic, max_rd_atomic);
	}

	if (mask & IB_QP_CUR_STATE)
		qp->attr.cur_qp_state = attr->qp_state;

	if (mask & IB_QP_EN_SQD_ASYNC_NOTIFY)
		qp->attr.en_sqd_async_notify = attr->en_sqd_async_notify;

	if (mask & IB_QP_ACCESS_FLAGS)
		qp->attr.qp_access_flags = attr->qp_access_flags;

	if (mask & IB_QP_PKEY_INDEX)
		qp->attr.pkey_index = attr->pkey_index;

	if (mask & IB_QP_PORT)
		qp->attr.port_num = attr->port_num;

	if (mask & IB_QP_QKEY)
		qp->attr.qkey = attr->qkey;

	if (mask & IB_QP_AV) {
		rxe_av_from_attr(rxe, attr->port_num, &qp->pri_av,
				 &attr->ah_attr);
	}

	if (mask & IB_QP_ALT_PATH) {
		rxe_av_from_attr(rxe, attr->alt_port_num, &qp->alt_av,
				 &attr->alt_ah_attr);
		qp->attr.alt_port_num = attr->alt_port_num;
		qp->attr.alt_pkey_index = attr->alt_pkey_index;
		qp->attr.alt_timeout = attr->alt_timeout;
	}

	if (mask & IB_QP_PATH_MTU) {
		qp->attr.path_mtu = attr->path_mtu;
		qp->mtu = rxe_mtu_enum_to_int((enum rxe_mtu)attr->path_mtu);
	}

	if (mask & IB_QP_TIMEOUT) {
		qp->attr.timeout = attr->timeout;
		if (attr->timeout == 0) {
			qp->qp_timeout_jiffies = 0;
		} else {
			int j = usecs_to_jiffies(4ULL << attr->timeout);
			qp->qp_timeout_jiffies = j ? j : 1;
		}
	}

	if (mask & IB_QP_RETRY_CNT) {
		qp->attr.retry_cnt = attr->retry_cnt;
		qp->comp.retry_cnt = attr->retry_cnt;
		rxe_debug(rxe, "set retry count = %d\n", attr->retry_cnt);
	}

	if (mask & IB_QP_RNR_RETRY) {
		qp->attr.rnr_retry = attr->rnr_retry;
		qp->comp.rnr_retry = attr->rnr_retry;
		rxe_debug(rxe, "set rnr retry count = %d\n", attr->rnr_retry);
	}

	if (mask & IB_QP_RQ_PSN) {
		qp->attr.rq_psn = (attr->rq_psn & BTH_PSN_MASK);
		qp->resp.psn = qp->attr.rq_psn;
		rxe_debug(rxe, "set resp psn = 0x%x\n", qp->resp.psn);
	}

	if (mask & IB_QP_MIN_RNR_TIMER) {
		qp->attr.min_rnr_timer = attr->min_rnr_timer;
		rxe_debug(rxe, "set min rnr timer = 0x%x\n",
			  attr->min_rnr_timer);
	}

	if (mask & IB_QP_SQ_PSN) {
		qp->attr.sq_psn = (attr->sq_psn & BTH_PSN_MASK);
		qp->req.psn = qp->attr.sq_psn;
		qp->comp.psn = qp->attr.sq_psn;
		rxe_debug(rxe, "set req psn = 0x%x\n", qp->req.psn);
	}

	if (mask & IB_QP_MAX_DEST_RD_ATOMIC) {
		qp->attr.max_dest_rd_atomic =
			__roundup_pow_of_two(attr->max_dest_rd_atomic);
	}

	if (mask & IB_QP_PATH_MIG_STATE)
		qp->attr.path_mig_state = attr->path_mig_state;

	if (mask & IB_QP_DEST_QPN)
		qp->attr.dest_qp_num = attr->dest_qp_num;

	if (mask & IB_QP_STATE) {
		qp->attr.qp_state = attr->qp_state;

		switch (attr->qp_state) {
		case IB_QPS_RESET:
			rxe_debug(rxe, "qp state -> RESET\n");
			rxe_qp_reset(qp);
			break;

		case IB_QPS_INIT:
			rxe_debug(rxe, "qp state -> INIT\n");
			qp->req.state = QP_STATE_INIT;
			qp->resp.state = QP_STATE_INIT;
			break;

		case IB_QPS_RTR:
			rxe_debug(rxe, "qp state -> RTR\n");
			qp->resp.state = QP_STATE_READY;
			break;

		case IB_QPS_RTS:
			rxe_debug(rxe, "qp state -> RTS\n");
			qp->req.state = QP_STATE_READY;
			break;

		case IB_QPS_SQD:
			rxe_debug(rxe, "qp state -> SQD\n");
			rxe_qp_drain(qp);
			break;

		case IB_QPS_SQE:
			rxe_error(rxe, "qp state -> SQE !!?\n");
			/* Not possible from modify_qp. */
			break;

		case IB_QPS_ERR:
			rxe_debug(rxe, "qp state -> ERR\n");
			rxe_qp_error(qp);
			break;
		}
	}

	return 0;
}

/* called by the query qp verb */
int rxe_qp_to_attr(struct rxe_qp *qp, struct ib_qp_attr *attr, int mask)
{
	struct rxe_dev *rxe = to_rdev(qp->ibqp.device);

	*attr = qp->attr;

	attr->rq_psn				= qp->resp.psn;
	attr->sq_psn				= qp->req.psn;

	attr->cap.max_send_wr			= qp->sq.max_wr;
	attr->cap.max_send_sge			= qp->sq.max_sge;
	attr->cap.max_inline_data		= qp->sq.max_inline;

	if (!qp->srq) {
		attr->cap.max_recv_wr		= qp->rq.max_wr;
		attr->cap.max_recv_sge		= qp->rq.max_sge;
	}

	rxe_av_to_attr(rxe, &qp->pri_av, &attr->ah_attr);
	rxe_av_to_attr(rxe, &qp->alt_av, &attr->alt_ah_attr);

	if (qp->req.state == QP_STATE_DRAIN) {
		attr->sq_draining = 1;
		msleep(1);
	} else {
		attr->sq_draining = 0;
	}

	rxe_debug(rxe, "attr->sq_draining = %d\n", attr->sq_draining);

	return 0;
}

/* called by the destroy qp verb */
void rxe_qp_destroy(struct rxe_qp *qp)
{
	struct rxe_dev *rxe = to_rdev(qp->ibqp.device);

	qp->valid = 0;
	qp->qp_timeout_jiffies = 0;
	rxe_cleanup_task(&qp->resp.task);

	if (qp->sq.queue) {
		/* only for non xrc rcv qp's */
		del_timer_sync(&qp->retrans_timer);
		del_timer_sync(&qp->rnr_nak_timer);

		rxe_cleanup_task(&qp->req.task);
		if (qp_type(qp) == IB_QPT_RC)
			rxe_cleanup_task(&qp->comp.task);
	}

	/* flush out any receive wr's or pending requests */
	__rxe_do_task(&qp->req.task);
	if (qp->sq.queue) {
		__rxe_do_task(&qp->comp.task);
		__rxe_do_task(&qp->req.task);
	}

	/* drain the output queue */
	while (!list_empty(&qp->arbiter_list))
		__rxe_do_task(&rxe->arbiter.task);
}

/* called when the last reference to the qp is dropped */
void rxe_qp_cleanup(void *arg)
{
	struct rxe_qp *qp = arg;

	rxe_drop_all_mcast_groups(qp);

	if (qp->sq.queue)
		rxe_queue_cleanup(qp->sq.queue);

	if (qp->srq)
		rxe_drop_ref(qp->srq);

	if (qp->rq.queue)
		rxe_queue_cleanup(qp->rq.queue);

	if (qp->xrcd)
		rxe_drop_ref(qp->xrcd);

	if (qp->scq)
		rxe_drop_ref(qp->scq);
	if (qp->rcq)
		rxe_drop_ref(qp->rcq);
	if (qp->pd)
		rxe_drop_ref(qp->pd);

	if (qp->resp.mr) {
		rxe_drop_ref(qp->resp.mr);
		qp->resp.mr = NULL;
	}

	free_rd_atomic_resources(qp);
}
