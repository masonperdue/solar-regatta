awk 'BEGIN { print "Time,Battery Current (A),SoC (%)" } /Battery:/ { print $1","$3","$5 }' 1.txt | sed '1!s/[A\%]//g' > data.csvwk 'BEGIN { print "Time Received,Battery Life (%),Voltage (V),Battery Current (A),Solar Current (A)" } /Time Received:/ { tr=$4$5 } /Battery Life:/ { bl=$4 } /Voltage:/ { v=$3 } /Battery Current:/ { bc=$4 } /Solar Current:/ { sc=$4; print tr","bl","v","bc","sc }' 1.txt | sed '1!s/[A%V]//g' > data.csv

awk 'BEGIN { print "Time,Battery Current (A),SoC (%)" } /Battery:/ { print $1","$3","$5 }' 1.txt | sed '1!s/[A\%]//g' > data.csv

