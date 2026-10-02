#!/usr/bin/env bash

# Load the project name from the Makefile
PROJECT_NAME=template
PROJECT_VERSION=f33
if [[ $(cat Makefile) =~ PROJECT_NAME[[:space:]]*:=[[:space:]]*([^[:space:]]*) ]]; then 
    PROJECT_NAME=${BASH_REMATCH[1]}
fi
if [[ $(cat Makefile) =~ PROJECT_VERSION[[:space:]]*:=[[:space:]]*([^[:space:]]*) ]]; then 
    PROJECT_VERSION=${BASH_REMATCH[1]}
fi

FNAME=$PWD/build/stm32/$PROJECT_NAME-$PROJECT_VERSION

echo "Programming with $FNAME"

mkfifo .uploader_telnet_fifo

echo -e "program ${FNAME}.elf verify reset\nexit" > .uploader_telnet_fifo &
telnet localhost 4444 < .uploader_telnet_fifo | grep -q "Connected to"
if [ $? == 0 ]; then 
    echo "Successfully uploaded with openocd"
    rm .uploader_telnet_fifo;
    exit 0; 
fi

echo -e "program -1 ${FNAME}.ihex\nexit" > .uploader_telnet_fifo &
telnet localhost 4445 < .uploader_telnet_fifo | grep -q "Connected to"
if [ $? == 0 ]; then 
    rm .uploader_telnet_fifo
    exit 0; 
fi

openocd -f ./openocd.cfg -c "program ${FNAME}.elf verify reset" -c "exit"
rm .uploader_telnet_fifo
