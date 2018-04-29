#!/usr/local/bin/bash

set -o pipefail 

cat "$1" | awk "{ print length(\$2), \$1, \$2 }" - | sort -n | awk "{ print \$3, \$1, \$2 }" - | uniq -cf 2 > "$2"
