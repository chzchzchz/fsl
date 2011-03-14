#!/bin/sh
sudo tunctl -u chz  -t tap1
sudo ifconfig tap1 up
sudo ifconfig tap1 10.0.0.10 # QEMU'S ETH0 INTERFACE SHOULD BE ON THE SAME CLASS C
