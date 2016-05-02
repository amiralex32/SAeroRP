#!/bin/sh

#NODES="10 20 30 40 50 60 70 80 90 100"
#TRIALS="1 2 3 4 5 6 7 8 9 10"
SEED="12345 23456 34567 45678 56789 67891 78912 89123 91234 10234"
SECURITYSTATUS="0 1"
NODES="10 20 30 40 50 60 70 80 90 100"
TRIALS="10"
echo Aero Experiment Example

pCheck=`which sqlite3`
if [ -z "$pCheck" ]
then
  echo "ERROR: This script requires sqlite3 (aero-test-final does not)."
  exit 255
fi

pCheck=`which gnuplot`
if [ -z "$pCheck" ]
then
  echo "ERROR: This script requires gnuplot (aero-test-final does not)."
  exit 255
fi

pCheck=`which sed`
if [ -z "$pCheck" ]
then
  echo "ERROR: This script requires sed (aero-test-final does not)."
  exit 255
fi

export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:bin/

# Remove existing database
if [ -e data.db ]
then
  echo "Kill data.db? (y/n)"
  read ANS
  if [ "$ANS" = "yes" -o "$ANS" = "y" ]
  then
    echo Deleting database
    rm data.db
  fi
fi

# Compile the simulation scenario with the different parameters, and run the number of trials
for nodes in $NODES
do
 export NS_GLOBAL_VALUE="RngRun=$trial"
  for securitystatus in $SECURITYSTATUS
   do
    for seed in $SEED
     do
      echo creating $nodes nodes, SEED $seed, SECURITYSTATUS $securitystatus
       export NS_GLOBAL_VALUE="RngSeed=$seed"
    ../../waf --run "aero-test-final --format=db --seed=$seed --nWifis=$nodes --securestatus=$securitystatus --run=run-$nodes-$seed-$securitystatus"
    done
  done
done
mv ../../data.db .

PDR_CMD="select exp.input,avg(((rx.value*100)/tx.value)) \
    from Singletons rx, Singletons tx, Experiments exp \
    where rx.run = tx.run AND \
          rx.run = exp.run AND \
          rx.variable='onoffRx' AND \
          tx.variable='onoffTx' AND \
          exp.strategy='0' \
    group by exp.input \
    order by abs(exp.input) ASC;"

SPDR_CMD="select exp.input,avg(((rx.value*100)/tx.value)) \
    from Singletons rx, Singletons tx, Experiments exp \
    where rx.run = tx.run AND \
          rx.run = exp.run AND \
          rx.variable='onoffRx' AND \
          tx.variable='onoffTx' AND \
          exp.strategy='1' \
    group by exp.input \
    order by abs(exp.input) ASC;"

#PDR_CMD="SELECT rx.run, avg(cast(rx.value as real)/cast(tx.value as real)) 
#		FROM Singletons rx, Singletons tx 
#		WHERE rx.variable = 'receiver-rx-packets' AND tx.variable='sender-tx-packets'
#		GROUP BY rx.run
#		ORDER BY rx.run ASC;"
# Create SQL command to get packet delivery ratio
#PDR_CMD="select exp.input,avg(100-((rx.value*100)/tx.value)) \
 #   from Singletons pdr, Experiments exp \
  #  where rx.run = tx.run AND \
  #        rx.run = exp.run AND \
  #        rx.variable='receiver-rx-packets' AND \
  #        tx.variable='sender-tx-packets' \
   # group by exp.input \
   # order by abs(exp.input) ASC;"

# Create SQL command to get end to end delay
#EED_CMD="select exp.input,(avg(delay.value)/1000000000) \
#    from Singletons delay, Experiments exp \
#    where delay.run = delay.run AND \
#          delay.run = exp.run AND \
#          delay.variable='receiver-rx-packets' AND \
#          delay.variable='sender-tx-packets' \
#    group by exp.input \
#    order by abs(exp.input) ASC;"

# Amount transmitted and received AeroRP packets AVG!
#AeroRPOHR_CMD="SELECT aerorpht.run, aerorpht.value, aerorphr.value, aerorpgt.value, aerorpgr.value 
#	FROM Singletons aerorpht, Singletons aerorphr,Singletons aerorpgt, Singletons aerorpgr
#	WHERE aerorpht.variable = 'helloTx' 
#        AND aerorphr.variable = 'helloRx' 
#        AND aerorpgt.variable = 'GSTx' 
#        AND aerorpgr.variable = 'GSRx'
#	GROUP BY aerorpht.run;"

sqlite3 -noheader data.db "$PDR_CMD" > AeroRPPDR.data
sqlite3 -noheader data.db "$SPDR_CMD" > SAeroRPPDR.data
#sqlite3 -noheader data.db "$EED_CMD" > AeroRPEED.data
#sqlite3 -noheader data.db "$OHR_CMD" > AeroRPOHR.data

sed -i "s/|/   /" AeroRPPDR.data
sed -i "s/|/   /" SAeroRPPDR.data
#sed -i "s/|/   /" AeroRPEED.data
#sed -i "s/|/   /" AeroRPOHR.data

# Get OnOff packet delay results
#DELAY_CMD="SELECT onoff.run, (avg(onoff.value)/1000000000) 
#	FROM Singletons onoff
#	WHERE onoff.variable = 'onoffDelay-average'
#	GROUP BY onoff.run;"

# Get everything!
#ALL_CMD="SELECT * FROM Singletons;"

# Query the SQLite Database
#sqlite3 -noheader datapdr.db "$PDR_CMD" > AeroRPPDR.data
#sqlite3 -noheader datadealy.db "$DELAY_CMD" > AeroRPDelay.data
#sqlite3 -noheader datadealy.db "$AERORP_CMD" > AeroRPOverHead.data

# Parse the data
#sed -i "s/run-//" AeroRP.data
#sed -i "s/|/   /" AeroRP.data

# Run gnuplot script and create graph
#gnuplot aero-delay.gnuplot
#gnuplot aero-overhead.gnuplot
gnuplot aero-pdr.gnuplot

echo "Done; data in AeroRP.data, plot in AeroRP.eps"
