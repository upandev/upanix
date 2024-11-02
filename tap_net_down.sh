#!/bin/sh

TAPDEV="$1"

ifconfig bridge1 deletem en0 deletem $TAPDEV
ifconfig bridge1 down

