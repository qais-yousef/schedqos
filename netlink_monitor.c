/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright (C) 2026 Qais Yousef */
#include <sys/socket.h>
#include <linux/netlink.h>
#include <linux/connector.h>
#include <linux/cn_proc.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "qos_manager.h"

/* Structure to wrap netlink message headers and connector payload */
struct __attribute__ ((aligned(NLMSG_ALIGNTO))) nlcn_msg
{
	struct nlmsghdr nlh;
	struct __attribute__ ((__packed__)) {
		struct cn_msg cn_msg;
		union {
			enum proc_cn_mcast_op mcast_op;
			struct proc_event proc_ev;
		};
	};
};

static int nl_connect(void)
{
	int nl_sock = socket(PF_NETLINK, SOCK_DGRAM, NETLINK_CONNECTOR);
	struct sockaddr_nl sa_nl = {0};
	int ret;

	if (nl_sock == -1) {
		perror("socket");
		return -1;
	}

	sa_nl.nl_family = AF_NETLINK;
	sa_nl.nl_groups = CN_IDX_PROC;
	sa_nl.nl_pid = getpid();

	ret = bind(nl_sock, (struct sockaddr *)&sa_nl, sizeof(sa_nl));
	if (ret == -1) {
		perror("bind");
		close(nl_sock);
		return -1;
	}
	return nl_sock;
}

/* Sends control message to kernel to start/stop listening */
static int set_proc_ev_listen(int nl_sock, int enable)
{
	struct nlcn_msg msg = {0};

	msg.nlh.nlmsg_len = sizeof(msg);
	msg.nlh.nlmsg_pid = getpid();
	msg.nlh.nlmsg_type = NLMSG_DONE;
	msg.cn_msg.id.idx = CN_IDX_PROC;
	msg.cn_msg.id.val = CN_VAL_PROC;
	msg.cn_msg.len = sizeof(enum proc_cn_mcast_op);
	msg.mcast_op = enable ? PROC_CN_MCAST_LISTEN : PROC_CN_MCAST_IGNORE;

	return send(nl_sock, &msg, sizeof(msg), 0);
}

void netlink_monitor(int nl_sock)
{
	struct nlcn_msg msg;

	while (1) {
		int status;

		status = recv(nl_sock, &msg, sizeof(msg), 0);
		if (status < 0) {
			LOG_ERROR("netlink got status: %d", status);
			continue;
		}

		switch (msg.proc_ev.what) {
		case PROC_EVENT_EXEC: {
			pid_t pid = msg.proc_ev.event_data.exec.process_pid;
			pid_t tgid = msg.proc_ev.event_data.exec.process_tgid;
			char comm[TASK_COMM_LEN] = {};

			get_comm_by_pid(pid, comm);

			LOG_VERBOSE("[EXEC] %s PID: %d, TGID: %d", comm, pid, tgid);

			create_app_instance(tgid);
			apply_thread_qos(pid, tgid, comm);
			break;
		} case PROC_EVENT_FORK: {
			pid_t ppid = msg.proc_ev.event_data.fork.parent_pid;
			pid_t ptgid = msg.proc_ev.event_data.fork.parent_tgid;
			pid_t pid = msg.proc_ev.event_data.fork.child_pid;
			pid_t tgid = msg.proc_ev.event_data.fork.child_tgid;
			char comm[TASK_COMM_LEN] = {};

			get_comm_by_pid(pid, comm);

			LOG_VERBOSE("[FORK] %s Parent: %d:%d, Child: %d:%d",
				    comm, ppid, ptgid, pid, tgid);

			/* Fork is another way to spawn new app */
			if (pid == tgid)
				create_app_instance(tgid);

			apply_thread_qos(pid, tgid, comm);
			break;
		} case PROC_EVENT_COMM: {
			pid_t pid = msg.proc_ev.event_data.comm.process_pid;
			pid_t tgid = msg.proc_ev.event_data.comm.process_tgid;
			char *comm = msg.proc_ev.event_data.comm.comm;

			LOG_VERBOSE("[COMM] PID: %d, TGID: %d, Name: %s", pid, tgid, comm);

			apply_thread_qos(pid, tgid, comm);
			break;
		} case PROC_EVENT_EXIT: {
			pid_t pid = msg.proc_ev.event_data.exit.process_pid;
			pid_t tgid = msg.proc_ev.event_data.exit.process_tgid;
			char comm[TASK_COMM_LEN] = {};

			get_comm_by_pid(pid, comm);

			LOG_VERBOSE("[EXIT] %s PID: %d, TGID: %d, Code: %d",
				    comm, pid, tgid,
				    msg.proc_ev.event_data.exit.exit_code);

			if (pid == tgid)
				destroy_app_instance(tgid);
			break;
		}
		default:
			LOG_VERBOSE("[PROC_EVT] Received event: %d", msg.proc_ev.what);
		}
	}
}

void start_netlink_monitor(void)
{
	int nl_sock = nl_connect();

	if (nl_sock == -1 || set_proc_ev_listen(nl_sock, 1) == -1)
		exit(1);

	LOG_VERBOSE("Listening for events...");
	netlink_monitor(nl_sock);
	close(nl_sock);
}
