#!/bin/bash

sed -e 's/ \+/ /g' $1.node > $1.node.simple
sed -i -e 's/^ //g' $1.node.simple

sed -e 's/ \+/ /g' $1.ele > $1.ele.simple
sed -i -e 's/^ //g' $1.ele.simple

./tetgen_convert $1
