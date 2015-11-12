#!/bin/bash

sed -e 's/ \+/ /g' $1.node > $2.node.simple
sed -i -e 's/^ //g' $2.node.simple

sed -e 's/ \+/ /g' $1.ele > $2.ele.simple
sed -i -e 's/^ //g' $2.ele.simple

./tetgen_convert $2 $2
