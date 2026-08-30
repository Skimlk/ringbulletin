#!/bin/sh

USER_NAME="$(logname)"
RINGBULLETIN_PATH="$(cd ..; pwd)"
echo "*/30 * * * * $USER_NAME cd $RINGBULLETIN_PATH && ./ringbulletin" > /etc/cron.d/ringbulletin
