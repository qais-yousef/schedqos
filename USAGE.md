Build:
======

```
make && sudo make install
```

There's a known compilation error due to bad handling of defining sched_attr
for systems that do have it in <sched.h>.

Start/stop/restart the daemon
=============================

```
sudo schedqos start --daemon
sudo schedqos stop
sudo schedqos restart --daemon
```

Log file will be stored in `/var/log/schedqosd.log`

Omit `--daemon` to start in fg.

Once run it'll modify sched_attr for all tasks starting from pid 1000 (still
deciding how to handle kernel threads) and this can't be undone without
a reboot yet.

You can modify the mapping and restart the service and it'll pick the new
changes and apply them.

Creating and applying configs
=============================

Easiest way is to add a config to `configs/apps_configs/my_config.json` and
do `sudo make install` again. Configs are stored in `/var/run/schedqos`. App
configs are assumed in `app_configs` subdirectory as json file.

Restart the daemon after any change to config files to pick up the new changes.

Example config:
---------------

This config will set all tasks for `chrome` as `user_interactive` and for
`make` as `background`.

```
{
	"chrome": {
		"period": 16000000,
		"qos": "QOS_USER_INTERACTIVE"
	},
	"make": {
		"qos": "QOS_BACKGROUND"
	}
}
```

Applying Sched Profiles
=======================

```
sudo schedqos sched_profile $profile
```

NOTE: profiles are set to use `schedutil`, which on some systems like
`intel_pstate` based ones requires setting the system in passive mode.
e.g: `echo passive | sudo tee /sys/devices/system/cpu/intel_pstate/status`

Available profiles
------------------

* throughput
* interactive
* qos

profiles description can be found in `configs/sched_profiles.json`. These are
preliminary WIP descriptions.

QoS Mappings
============

QoS levels are mapping to sched_attr values in `configs/qos_mappings.json`. You
should be able to add any existing sched_attr name to the setup and the code
will cope with applying it.
