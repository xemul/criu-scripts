#!/bin/bash

DIR=$(dirname ${BASH_SOURCE[0]})

wwtarg=""
sortag="-nr"
headcmd="cat"

while :; do
	case "$1" in
	"--people"|"-p")
		shift
		;;
	"--company"|"-C")
		wwtarg="--company"
		shift
		;;
	"--commits"|"-c")
		sortag="-k4nr"
		shift
		;;
	"--lines"|"-l")
		shift
		;;
	"--top"|"-t")
		shift
		headcmd="head -n$1"
		shift
		;;
	"--help"|"-h")
		echo "Options:"
		echo -e "\t-p|--people      show people stats (defaul)"
		echo -e "\t-C|--company     show company stats"
		echo -e "\t-l|--lines       sort by lines (default)"
		echo -e "\t-c|--commits     sort by commits"
		echo -e "\t-t|--top NUM     show top NUM records"
		exit 0
		;;
	*)
		break
		;;
	esac
done

echo "              Lines added           Commits   Who"

git log --pretty=short --shortstat --no-merges $@ | \
		 sh ${DIR}/criu-name-map | ${DIR}/wwt.py $wwtarg | \
		 sort $sortag | $headcmd | nl
