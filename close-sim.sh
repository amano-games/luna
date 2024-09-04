#! /bin/bash
SIM_PID=$(pgrep -f 'PlaydateSimulator')

echo "$SIM_PID" | xargs -r kill -9
