#!/bin/sh


#UTENTI="3 4 17 30 41 62 68 128 153 163"
UTENTI="1 2 3 4 7 8 9 10 12"


PROP="100"

for USER in $UTENTI
do
	nohup ./gigeo $USER $PROP ../../yonsei.db > "log_"$USER"_"$PROP"perc.txt" 2>&1 &
done
