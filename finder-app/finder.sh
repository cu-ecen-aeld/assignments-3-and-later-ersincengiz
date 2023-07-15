#!/bin/sh


filesdir=$1
searchstr=$2

#Are the arguments empty?
if [ ! -z $filesdir ] && [ ! -z $searchstr ];
then
    if [ -d $filesdir ] #FILE exists and is a directory.
	then
	#Returns the number of lines containing the data.
	Y="$(find "$filesdir" | xargs grep -oh "$searchstr" | wc -w)"
	#Returns the number of files containing the data.	
	X="$(find "$filesdir" | xargs grep -l "$searchstr" | wc -l)"
	echo "The number of files are $X and the number of matching lines are $Y"
    else
	exit 1
     fi
else
	exit 1
fi
