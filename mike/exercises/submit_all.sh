#!/bin/bash
# Sottomette alla coda PBS tutti gli esercizi, uno dopo l'altro.

# Elenco delle cartelle degli esercizi
DIRS=(
    "Exercise1:Communication"
    "Exercise2:Communication"
    "Exercise3:PingPong"
    "Exercise4:OpenClosedRing"
    "Exercise5:Reduce"
    "Exercise6:AllReduce"
    "Exercise7:PingPongAd"
    "Exercise8:BroadcastTreeStructured"
)

echo "================================================="
echo "  Sottomissione di tutti gli esercizi a PBS...   "
echo "================================================="

for dir in "${DIRS[@]}"; do
    if [ -d "$dir" ]; then
        echo "--> Entro nella cartella: $dir"
        cd "$dir" || continue
        
        # Sottometti lo script solo se esiste
        if [ -f "run.sh" ]; then
            echo "   Sottometto $dir/run.sh..."
            qsub run.sh
        else
            echo "   [ATTENZIONE] run.sh non trovato in $dir"
        fi
        
        # Torna alla cartella principale
        cd ..
    else
        echo "   [ERRORE] Cartella $dir non trovata"
    fi
    echo "-------------------------------------------------"
done

echo "Sottomissione completata! Controlla lo stato con 'qstat -u $USER'."
