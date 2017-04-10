#!/bin/bash
host=$1
port=$2
#echo "Input File:"
#echo -e "$inputFile"
for i in {1..10}
do
	#echo -e "$inputFile" | nc "$host" "$port"
	echo -e "\\x00\\x00\\x00\\x00\\x00\\x00\\x03\\x2C<?xml version=\"1.0\" encoding=\"UTF-8\"?><transactions reset=\"true\"><create ref=\"c1\"><account>1234</account><balance>500</balance></create><create ref=\"c2\"><account>5678</account></create><create ref=\"c3\"><account>1000</account><balance>500000</balance></create><create ref=\"c4\"><account>1001</account><balance>5000000</balance></create><transfer ref=\"1\"><to>1234</to><from>1000</from><amount>9568.34</amount><tag>paycheck</tag><tag>monthly</tag></transfer><transfer ref=\"2\"><from>1234</from><to>1001</to><amount>100.34</amount><tag>food</tag></transfer><transfer ref=\"3\"><from>1234</from><to>5678</to><amount>345.67</amount><tag>saving</tag></transfer><balance ref=\"xyz\"><account>1234</account></balance><query ref=\"4\"><or><equals from=\"1234\"/><equals to=\"5678\"/></or><greater amount=\"100\"/></query></transactions>" | nc "$host" "$port" >> "/dev/null"
done
