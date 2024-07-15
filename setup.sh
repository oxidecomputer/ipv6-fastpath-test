#!/bin/bash

pkg install brand/sparse

mkdir -p /tmp/zone1
mkdir -p /tmp/zone2


zfs create -o mountpoint=/v6-lso-test rpool/v6-lso-test

dladm create-vnic -l vioif0 vnic1
dladm create-vnic -l vioif0 vnic2

pkg set-publisher --search-first helios-dev

zonecfg -z zone1 -f zone1.txt
zoneadm -z zone1 install
zoneadm -z zone1 boot

zonecfg -z zone2 -f zone2.txt
zoneadm -z zone2 install
zoneadm -z zone2 boot

pkg set-publisher --search-first on-nightly
