#!/bin/bash

cd tests;
for (( i=0; i<100; i++))
do

    echo -e "\n\nESECUZIONE NUMERO $i"   
    ./test.sh 
    # if ./test.sh | grep "failed" ; then
    #     echo "ESECUZIONE $i , fallita"
    #     break;
    # fi


    
done