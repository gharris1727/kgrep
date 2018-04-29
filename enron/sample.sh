#!/usr/local/bin/bash

set -o pipefail 

shuffle -f "$1" -p 1000 | cut -wf 3 > "$2"
