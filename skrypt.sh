#!/bin/bash

COUNTER=60
declare -a Task=(":cat /null.txt | wc -l:0" ":ls -l:2" ":cat /null.txt | wc -l:2" ":ls -l | tail -a:2" ":cat /null.txt:1" ":ls -l | tail -n5 | head -n2:2")
mkdir -p ~/Minicron
rm ~/Minicron/taskfile.txt 2> /dev/null

for i in "${Task[@]}"
do
	time=`date --date="+$COUNTER seconds" +"%H:%M"`;
	echo "$time$i" >> ~/Minicron/taskfile.txt
	COUNTER=$((COUNTER+60))
done

echo "Wygenerowano ~/Minicron/taskfile.txt"

./minicron ~/Minicron/taskfile.txt ~/Minicron/outfile.txt

