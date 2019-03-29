#!/bin/bash

cd bin
perf report -g "graph,0.5,caller" --no-children
