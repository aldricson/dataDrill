#!/bin/bash

top -b -n 2 -d 1 | grep '\./dataDrill' | awk '{print $7}' | tr -d '%' | sed -n '3p'
