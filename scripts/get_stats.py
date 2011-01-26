#!/usr/bin/python

import os
import re
import subprocess
import time

# Get uptime in minutes from /proc/uptime
def get_uptime():
    if not os.path.exists("/proc/uptime"):
        return "-"
    f = open('/proc/uptime', 'r')
    row = f.readline()
    f.close()
    uptime = int(round(float(row.split()[0]) / 60))
    return uptime


# Get 10x load average from /proc/loadavg
def get_loadavg():
    if not os.path.exists("/proc/loadavg"):
        return "-"
    f = open('/proc/loadavg', 'r')
    row = f.readline()
    f.close()
    loadavg = int(round(float(row.split()[0]) * 10))
    return loadavg


# Get wireless signal strength (0-100%)
# FIXME: assumes "wlan0" and "Link Quality=39/70" string for the strength
def get_wlan_signal():
    if not os.path.exists("/sbin/iwconfig"):
        return "-"
    output = subprocess.Popen(["/sbin/iwconfig", "wlan0"], stdout=subprocess.PIPE).communicate()[0]
    signal = re.search('(?<=Link Quality=)\d+/\d+', output)

    values = signal.group(0).split("/");

    if signal == None:
        return "-"

    return int(round((float(values[0]) / float(values[1])) * 100))

while True:
    print get_uptime(), get_loadavg(), get_wlan_signal()
    time.sleep(1)
