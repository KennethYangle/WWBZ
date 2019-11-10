#!/bin/bash
sudo ./build/droneyee serial:///dev/ttyS0:460800 | tee ./log/`date +%y%m%d-%H:%M:%S`.log
