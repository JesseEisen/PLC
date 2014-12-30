#!/bin/bash

datetime=$(date +%Y%m%d-%H%M%S)
if [ -s sensordata.txt ]; then
	cp sensordata.txt /backup/$datetime.txt
fi
echo '' > sensordata.txt
echo "backup complete..."
sudo ./Serial
echo "Restart Successfully, Getting data... Input ctrl+c to interrupt"
