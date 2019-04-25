#!/bin/bash

dir=$1

function eps2png_convert()
{
    eps=$1
    png="${eps%.eps}.png"
    convert $eps $png
    rm $eps
}
export -f eps2png_convert

find $dir -name "*eps" | parallel -j 4 eps2png_convert {}
