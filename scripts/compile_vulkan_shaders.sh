#!/usr/bin/env bash

# usage: ./compile_shaders.sh path/to/input/shaders path/to/output/shaders

if [ "$#" -ne 2 ]
then
	echo "incorrect number of arguments passed. skipping.."
	exit 2
fi

if [ ! -d $1 ] 
then
	echo "input shaders directory "$1" doesn't exist. skipping.."
	exit 2
fi

if [ ! -d $2 ]
then
	echo "creating directory "$2".."
	mkdir $2
fi

rm $2/*

error=0
num_shaders=0

for shader in $1/*
do
	bn="$(basename $shader)"
	if ! glslc $shader -o $2"/"$bn".spv"
	then
		error=1
	else
		num_shaders=$((num_shaders+1))
	fi
done

if [ $error -ne 1 ]
then
	echo $num_shaders" shaders successfully compiled."
fi
