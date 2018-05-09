
set -f

TIMEFORMAT="%3R,%3U,%3S"
PAT="$(pwd)/enron/obj/test_words"
RES=$(pwd)/enron.csv
IN="$(pwd)/enron/obj/enron.tar"
OUT="/tmp/enron_out"

truncate -s 0 "$RES"
truncate -s 0 "$OUT"

echo "length,matches,program,real,user,system" >> $RES

function run_test {
    # $1 is the pattern to test
    # $2 is the length of the pattern
    # $3 is the number of matches that appear in the file
    printf "%s,%s,gnu_grep," "$2" "$3" >> $RES
    { time gnugrep/grep31 -aoFe "$1" "$IN" > "$OUT"; } 2>> $RES
    truncate -s 0 $OUT
    printf "."
    printf "%s,%s,kacc_grep," "$2" "$3" >> $RES
    { time kagrep/cli/obj/kagrep "$1" fgrep ao "$IN" "$OUT"; } 2>> $RES
    truncate -s 0 $OUT
    printf ".\n"
}

declare -a count
declare -a pattern
declare -a length
for s in `cat "$PAT"`; do
    count+=($(grep -cFe "$s" "$IN"))
    pattern+=("$s")
    length+=(${#s})
    i=$((${#count[*]} - 1))
    printf "%s\t%s\t%s\n" "${length[$i]}" "${count[$i]}" "${pattern[$i]}"
done

for mult in `seq 1 1 20`; do
    i=0
    while [ $i -lt ${#count[*]} ]; do
        printf "%s\t%s\t%s\t" "$mult" "${length[$i]}" "${count[$i]}" "${pattern[$i]}"
        time run_test "${pattern[$i]}" "${length[$i]}" "${count[$i]}"
        i=$(( $i + 1))
    done
done

cat $RES
