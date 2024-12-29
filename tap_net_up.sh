#!/bin/sh

TAPDEV="$1"

ifconfig bridge1 create
ifconfig bridge1 addm en0 
ifconfig bridge1 addm $TAPDEV
ifconfig bridge1 up

