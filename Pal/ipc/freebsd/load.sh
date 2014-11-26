#!/bin/sh

module="graphene-ipc"

(/sbin/kldstat | grep -q "graphene_ipc") && \
((echo "unloading graphene_ipc..."; /sbin/kldunload graphene_ipc) || exit 1) || continue

# invoke insmod with all arguments we got
# and use a pathname, as newer modutils don't look in . by default
echo "loading graphene_ipc..."
/sbin/kldload ./$module.ko $* || exit 1
