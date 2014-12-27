#!/bin/bash
gcc -c 32kb_8way.s -o 32kb_8way.o
objdump -d 32kb_8way.o > 32kb_8way.dump
