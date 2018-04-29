#!/usr/local/bin/bash

set -o pipefail 

cat "$1" | tr -c "[[:print:]]" "\n" | tr " \n\t," "\n" |  grep -axv "" | sort | uniq -c > "$2"
