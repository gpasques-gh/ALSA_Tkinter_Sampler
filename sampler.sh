#!/usr/bin/env bash
echo "building project..."
make clean
make CONTROLLER=AKAI_MPK
echo "launching C sampler..."
./bin/sampler &
sleep 1
echo "launching Python GUI..."
python3 python/sampler.py
