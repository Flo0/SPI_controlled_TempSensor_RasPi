#!/bin/bash

echo "Compiling main.c with gcc to prog."
gcc -pthread -o prog -Wall main.c -L. -lwiringPiSim

echo "Adding execute permission to prog."
sudo chmod +x ./prog

echo "Starting temp sensor"
x-terminal-emulator -e python3 ./temp_simulation.py

echo "Starting main program"
echo ""
sudo ./prog

echo "I have no idea how to get the python id from here"
echo "so you have to close it yourself :("