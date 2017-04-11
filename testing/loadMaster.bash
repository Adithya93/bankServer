#!/bin/bash
for i in {1..50} 
do
	./makeReqsNoQueryResults.bash $1 $2 &
done
wait
