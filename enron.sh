
TIMEFORMAT=",%3R,%3U,%3S"
PAT="$HOME/enron/test_words"
RES=$(pwd)/enron.csv
IN="$HOME/enron/enron.tar"
OUT="/dev/null"

truncate -s 0 $RES

echo "length,matches,program,real,user,system" >> $RES

function run_test {
    # $1 is the pattern to test
    length=${#1}
    printf "%s %s" $length $1
    count=$(grep -c "$1" "$IN")
    printf " %s\n" $count
    printf "%s,%s,gnu_grep" $length $count >> $RES
    { time grep -aoF "$1" "$IN" > "$OUT"; } 2>> $RES
    printf "%s,%s,kacc_grep" $length $count >> $RES
    { time kagrep/cli/kagrep "$1" fgrep ao "$IN" "$OUT"; } 2>> $RES
}

for mult in `seq 1 1 20`; do
    for pattern in `cat $PAT`; do
        run_test $pattern
    done
done

cat $RES
