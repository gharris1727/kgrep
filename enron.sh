
set -f

TIMEFORMAT="%3R,%3U,%3S"
PAT="$(pwd)/enron/obj/test_words"
RES=$(pwd)/enron.csv
IN="$(pwd)/enron/obj/enron.tar"
OUT="/dev/null"

truncate -s 0 $RES

echo "length,matches,program,real,user,system" >> $RES

function run_test {
    # $1 is the pattern to test
    length=${#1}
    count=$(grep -cFe "$1" "$IN")
    printf "%s\t%s\t%s " "$length" "$count" "$1"
    printf "%s,%s,gnu_grep," "$length" "$count" >> $RES
    { time grep -aoFe "$1" "$IN" > "/tmp/enron_grep_out"; } 2>> $RES
    printf "."
    truncate -s 0 /tmp/enron_grep_out
    printf "%s,%s,kacc_grep," "$length" "$count" >> $RES
    { time kagrep/cli/obj/kagrep "$1" fgrep ao "$IN" "$OUT"; } 2>> $RES
    printf ".\n"
}

for mult in `seq 1 1 10`; do
    for pattern in `cat $PAT`; do
        printf "%s\t" "$mult"
        time run_test "$pattern"
    done
done

cat $RES
