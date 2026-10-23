#!/bin/bash
# Wrapper eseguito da mpirun su ogni rank: forza UCX (la libreria di trasporto
# usata da OpenMPI) a comunicare solo via Ethernet/TCP, sull'interfaccia di rete
# che porta alla rete del cluster.

# Trova l'interfaccia di rete usata per raggiungere 192.168.108.1:
# "ip route get" stampa la rotta e awk prende la parola che segue "dev".
IFACE=$(ip -4 route get 192.168.108.1 |
        awk '{for (i=1;i<=NF;i++) if ($i=="dev") {print $(i+1); exit}}')

# Se non si trova nessuna interfaccia (o e' il loopback "lo") non ha senso proseguire
if [[ -z "$IFACE" || "$IFACE" == "lo" ]]; then
    echo "ERRORE: nessuna interfaccia Ethernet valida trovata su $(hostname)" >&2
    exit 1
fi

# Trasporti UCX ammessi: tcp (rete), self (stesso processo), sm (memoria condivisa)
export UCX_TLS=tcp,self,sm
# Usa esclusivamente l'interfaccia trovata sopra
export UCX_NET_DEVICES="$IFACE"

# Messaggio di debug (finisce nel file .err): mostra quale interfaccia usa ogni nodo
echo "$(hostname): UCX_NET_DEVICES=$UCX_NET_DEVICES" >&2

# Sostituisce questo script con il programma vero (passato come argomenti)
exec "$@"
