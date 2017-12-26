#!/bin/bash

trap 'kill $(jobs -p)' EXIT

if [ -z "${PLECO_EXEC}" ]
then
	PLECO_EXEC="slave"
fi

while true
do
	"${PLECO_EXEC}" $*
	sleep 1
done
