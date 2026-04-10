Kernel Changes
==============

* Enable NETLINK by default in kernel KConfig
* Add new NETLINK event to inform userspace when a task changes cgroup
* Add kernel sched_qos interface
* Add rampup_multiplier to manage DVFS latencies via new QoS interface
* Make wake up path latency aware
* Make wake up path idle states aware
* Support push load balancer that re-uses select_task_rq() for placement
  decision
* Enable run time enable/disable of autogroup and cgroup cpu controller
* Introduce provisions to implement unfair PI lock
* Lock sched_attr via special new CAP (or selinux in userspace)

Userspace changes
=================

* Modernize the build system for easier integration with distros
* Enable globbing in app name and thread_qos tasks name
* Support specifying the full path for the binary/process
* More flexible qos tags based purely on definition from config file
* Proper support for scaling based on provided PERIOD
* Tune debugfs knobs for all sched profiles
* Add listening for cgroup attach/detach and allow specifying an policy based
  on group name
* Enable PTHREAD_PRIO_INHERIT by default in libc
* Lock sched_attr via selinux (or via new CAP in kernel)
