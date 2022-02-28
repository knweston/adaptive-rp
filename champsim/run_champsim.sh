#!/bin/bash
if [ "$#" -lt 4 ]; then
    echo "Illegal number of parameters"
    echo "Usage: ./run_champsim.sh [BINARY] [N_WARM] [N_SIM] [CONFIG] [TRACE] [OPTION]"
    exit 1
fi

BINARY=${1}
N_WARM=${2}
N_SIM=${3}
CONFIG=${4}
TRACE=${5}
OPTION=${6}

# Sanity check
if [ ! -f "$BINARY" ] ; then
    echo "[ERROR] Cannot find a ChampSim binary: $BINARY"
    exit 1
fi

re='^[0-9]+$'
if ! [[ $N_WARM =~ $re ]] || [ -z $N_WARM ] ; then
    echo "[ERROR]: Number of warmup instructions is NOT a number" >&2;
    exit 1
fi

re='^[0-9]+$'
if ! [[ $N_SIM =~ $re ]] || [ -z $N_SIM ] ; then
    echo "[ERROR]: Number of simulation instructions is NOT a number" >&2;
    exit 1
fi

if [ ! -f "$TRACE" ] ; then
    echo "[ERROR] Cannot find a trace file: $TRACE"
    exit 1
fi

mkdir -p results_${N_SIM}M
(${BINARY} -warmup_instructions ${N_WARM}000000 -simulation_instructions ${N_SIM}000000 ${OPTION} -config ${CONFIG} -traces ${TRACE})  &>> results_${N_SIM}M/${TRACE:39:3}-${BINARY:53}.txt
# (${BINARY} -warmup_instructions ${N_WARM}000000 -simulation_instructions ${N_SIM}000000 ${OPTION} -config ${CONFIG} -traces ${TRACE})  &>> results_${N_SIM}M/${TRACE:37:3}-${BINARY:4}.txt
