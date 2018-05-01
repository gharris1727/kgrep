#!/usr/local/bin/bash

set -o pipefail 

shuffle -f "$1" -p 500 | cut -wf 3 > "$2"
