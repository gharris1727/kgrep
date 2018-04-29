
TIMEFORMAT=",%3R,%3U,%3S"
RES=$(pwd)/urandom.csv
IN="/dev/urandom"
OUT="/dev/null"

truncate -s 0 $RES

echo "pattern,matches,program,real,user,system" >> $RES

function run_test {
    # $1 is the pattern to test
    # $2 is the number of matches to find
    printf "%s,%s,gnu_grep" $1 $2 >> $RES
    { time grep -aoE $1 $IN > $OUT -m $2; } 2>> $RES
    printf "%s,%s,kacc_grep" $1 $2 >> $RES
    { time kagrep/cli/obj/kagrep $1 egrep ao $IN $OUT $2; } 2>> $RES
    printf "%s,%s,user_grep" $1 $2 >> $RES
    { time ugrep/obj/ugrep $1 $IN $OUT $2; } 2>> $RES
    printf "%s,%s,kern_grep" $1 $2 >> $RES
    { time kgrep/cli/obj/kgrep $1 $IN $OUT $2; } 2>> $RES
}

MAX_PLENGTH=3

for mult in `seq 1 1 20`; do
    for input in `seq 1 1 10`; do
        for plength in `seq $MAX_PLENGTH`; do
            PATTERN=$(for i in `seq $plength`; do 
                echo -n "a" 
            done)
            MATCHES_EXPR="$input*(256^($MAX_PLENGTH-$plength))"
            MATCHES=$(echo $MATCHES_EXPR | bc)
            echo $mult $plength $input $PATTERN $MATCHES
            run_test $PATTERN $MATCHES
        done
    done
done

cat $RES
