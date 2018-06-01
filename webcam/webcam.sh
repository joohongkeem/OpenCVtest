#!/bin/bash

DATE=$(date +"%Y-%m-%d_%H%M%S")

fswebcam -r 640x480 --no-banner /home/pi/webcam/$DATE.jpg
