#!/bin/bash

S2PID=0
TMP_FILE=""

if [ -z "$SLEEP_TIME" ]; then
	SLEEP_TIME=5
fi

cleanup_and_exit() {

	S2PID="`pgrep sikso2`"
	if ! [ -z "$S2PID" ]; then
		kill -9 $S2PID > /dev/null 2>&1
	fi

	rm -rf "$TMP_FILE"

	echo "(i) Interrupted by user..."

	exit 0
}

echo -ne "(i) Building with config_perf_tests.txt..."

make clean > /dev/null
make CONFIG_FILE=tests/config_perf_tests.txt > /dev/null

echo " Done!"

TMP_FILE="`mktemp`"

echo -ne "(i) Running test.asm for ${SLEEP_TIME}s..."

./sikso2 -r test.asm > "$TMP_FILE" &
S2PID=`echo $!`

trap cleanup_and_exit INT

sleep $SLEEP_TIME

echo " Done!"

kill -9 $S2PID > /dev/null 2>&1
S2PID=0

echo -ne "(i) Parsing (this could take a few minutes)..."

SED_RES="`sed -n 's/Cycle lasted \([0-9]\+\)ns\./\1/p' "$TMP_FILE"`"

echo " Done!"

echo -ne "(i) Calculating average time..."

AVG="`echo "$SED_RES" | awk 'BEGIN { num=0; sum=0; } { num++; sum = sum + $0; } END { print (sum / num) }'`"

echo " Done!"

rm -rf "$TMP_FILE"

echo "(i) Average time is ${AVG}ns per instruction."

exit 0
