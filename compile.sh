#!/bin/bash
clang -g -O0 -Wall -Wextra -Iminilib *.c minilib/*.c -lm -o ping
sudo chown root:root ping
sudo chmod u+s ping
