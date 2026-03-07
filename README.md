Sched QoS
=========

Sched QoS is a utility/library supported and maintained by the sched community
to enable easy configuration of scheduler behavior. It provides a higher level
QoS tags to describe applications behavior that will be translated to
whatever sched attributes required to achieve that behavior. The goal is to
detach application description from scheduler specifics; to maintain
scalability and portability for that description to Just Work across kernels
and hardware.

For usage, check this [guide](USAGE.md)

QoS Tags
========

QoS Tags describe the role of the task. The model is based on existing one
shipping by mac.

* User Interactive
* User Initiated
* Utility
* Background

User Interactive
----------------

Tasks that perform user visible interactions, or alike, and require near
immediate response.

User Initiated
--------------

Tasks that perform user visible actions that can tolerate some short delays,
but not prolonged ones.

Utility
-------

Tasks that perform non user visible work that can tolerate longer delays.

Background
----------

Tasks with no performance impact. Can tolerate prolonged delays.

Execution Profiles
==================

**Not Implemented: RFC**

Linux has hardware variety that is not seen by other operating systems. To
enable better use of hardware resources, some information about the execution
profile of the task can help to take advantage of cache hierarchy and
potentially HMP systems.

* Compute Bound
* Memory Bound
* Synchronization Bound

Compute Bound
-------------

The number cruncher. Highest IPC is desirable.

Memory Bound
------------

The data manipulator. There can be multiple flavours

- Cache sensitive: small data set; prefers staying on one cpu
- Cache streaming/thrashing: large data set, can thrash other tasks cache
  locality
- NUMA sensitive: pointer chasers, prefers to stay within the same LLC

Synchronization Bound
---------------------

- Producer/Consumer: both tasks need to keep each others busy and avoid waiting
- IO: same as producer/consumer where the consumer is a device

Access Controls
===============

**Not Implemented: TODO**

We need new netlink events to tell us when a task changes cgroups so that we
can implement a userspace access control rules.

```
{
	"foreground": {
		"QOS_USER_INTERACTIVE": true
	},
	"background": {
		"QOS_USER_INTERACTIVE": false
	}
}
```

Applying a configuration requires schedqos utility to run as a privilged user,
so only admins can apply tweaks and apply configurations.

We need new CAP_PERF_MANAGER to block tasks from modifying sched_attr and
disturb the harmony of the system.

Sched Profiles
==============

Scheduler default behavior matters. Many systems expects it to suite their
needs out of the box without any additional hints or tuning. But given the
diversity of hardware and workloads Linux deals with, it is had to provide
a single default behavior that suits every body.

The sched profile intend to solve this problem. There are many knobs in debugfs
that help modify this default behavior, but require expert knowledge sometimes.

The profiles abstract these behavior into a known set of behavior that users
want:

* Throughput
* Interactive
* QoS

Throughput
----------

For systems that need the best number crunching. Time spent performing
scheduler decisions is a wasted time. Minimal context switching overhead and
long time slices are key elements of this profile.

Interactive
-----------

For systems that are sensitive to latencies. General desktop behavior is a good
example where performance, power and latencies all matter and a well balanced
behavior is desired.

These systems are characterized with more context switches to allow progress
for the various workloads competing for resources. More time spent in scheduler
making better decisions is not perceived as a problem in general.

QoS
---

When users decide to opt-in to provide hints to the scheduler about their
needs, this profile is recommended to keep the system noise to a minimum.

These systems are managed by a centralized schedqos daemon that manages QoS
request to provide a desired behavior for a task or process.

By default the scheduler will assume all tasks are not important to reduce the
noise floor of the various activities of the system, and enable providing
better best effort QoS service.

This profile will also lock down sched_attr syscall so that tasks can't disturb
the harmony of the system by breaking decision made by the centraze schedqos
daemon.

Config based hinting
====================

Instead of providing APIs that requires apps to be rewritten and re-distributed
for users to see the benefit. A config based hinting system was created to
enable easy tagging of tasks to give them a desired behavior.

Example
-------

`app_a` was profile and two tasks were identified as worthy of tagging. `taskA`
is `QOS_USER_INTERACTIVE` and requires short latency and fast DVFS response
times. `taskB` is a busy background task that can consume unnecessary
resources and power/performance can be improved by tagging it as `background`
task.

`app_b` wasn't profiled but user wants to ensure it gets good performance
regardless, all tasks are set to default `QOS_USER_INTERACTIVE` to guarantee
best performance even if this leads to wasted power or higher contention with
other `QOS_USER_INTERACTIVE` tasks.

```
{
	"app_a": {
		"period": 16,
		"thread_qos": {
			"taskA": "QOS_USER_INTERACTIVE",
			"taskB": "QOS_BACKGROUND"
		}
	},
	"app_b": {
		"period": 16,
		"qos": "QOS_USER_INTERACTIVE"
	}
}
```
