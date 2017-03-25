#!/bin/bash

trap 'kill $(jobs -p)' EXIT

while true
do
	slave $*
	sleep 1
done
