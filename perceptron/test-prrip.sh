#!/bin/bash
#SBATCH --job-name=prrip
#SBATCH --output=/scratch/user/kevin.weston/data/result/prrip-%j.txt
#SBATCH --time=12:00:00
#SBATCH --nodes=1
#SBATCH --ntasks-per-node=6
#SBATCH --mem=16G

./run_champsim.sh /scratch/user/kevin.weston/adaptive-rp/perceptron/bin/prrip-0.1 0 1000 /scratch/user/kevin.weston/dpc3-traces/602.champsimtrace.xz &
./run_champsim.sh /scratch/user/kevin.weston/adaptive-rp/perceptron/bin/prrip-0.1 0 1000 /scratch/user/kevin.weston/dpc3-traces/605.champsimtrace.xz &
./run_champsim.sh /scratch/user/kevin.weston/adaptive-rp/perceptron/bin/prrip-0.1 0 1000 /scratch/user/kevin.weston/dpc3-traces/620.champsimtrace.xz &
./run_champsim.sh /scratch/user/kevin.weston/adaptive-rp/perceptron/bin/prrip-0.1 0 1000 /scratch/user/kevin.weston/dpc3-traces/621.champsimtrace.xz &
./run_champsim.sh /scratch/user/kevin.weston/adaptive-rp/perceptron/bin/prrip-0.1 0 1000 /scratch/user/kevin.weston/dpc3-traces/619.champsimtrace.xz
sleep 1200