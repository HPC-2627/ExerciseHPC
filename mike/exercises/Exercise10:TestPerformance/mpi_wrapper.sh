#!/bin/bash
# Wrapper run by mpirun on every rank: forces UCX (the transport library
# used by OpenMPI) to communicate only via Ethernet/TCP, on the network interface
# that leads to the cluster network.

# Find the network interface used to reach 192.168.108.1:
# "ip route get" prints the route and awk takes the word following "dev".
IFACE=$(ip -4 route get 192.168.108.1 |
        awk '{for (i=1;i<=NF;i++) if ($i=="dev") {print $(i+1); exit}}')

# If no interface is found (or it is the loopback "lo") there is no point in continuing
if [[ -z "$IFACE" || "$IFACE" == "lo" ]]; then
    echo "ERRORE: nessuna interfaccia Ethernet valida trovata su $(hostname)" >&2
    exit 1
fi

# Allowed UCX transports: tcp (network), self (same process), sm (shared memory)
export UCX_TLS=tcp,self,sm
# Use exclusively the interface found above
export UCX_NET_DEVICES="$IFACE"

# Debug message (ends up in the .err file): shows which interface each node uses
echo "$(hostname): UCX_NET_DEVICES=$UCX_NET_DEVICES" >&2

# Replace this script with the real program (passed as arguments)
exec "$@"
