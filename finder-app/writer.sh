#!/bin/bash


writefile=$1
writestr=$2

if [ ! -z $writefile ] && [ ! -z $writestr ];
then
    #if [ -d $writefile ]
	#then
	    if [ -e $writefile ]
		then
		 echo $writestr >> $writefile
	    else
	    if [ ! -d $writefile ] 
	    then
	    part1=$(dirname "$writefile")
	        mkdir -p $part1
	    fi
		touch  "$writefile" 2> /dev/null
		if [ $? -eq 0 ] 
		then 
		 echo $writestr > $writefile
		else 
		  echo "Could not create file" >&2 
		  echo "$writefile"
		fi

	    fi
#    else
 #   	echo "filesdir does not represent the file directory."
 #   	exit 1
  #  fi
else
   exit 1
fi
