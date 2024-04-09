#!/bin/bash
try(){
    expected="$1"
    input="$2"

    ./9cc "$input" > tmp.s
    gcc -o tmp tmp.s
    ./tmp
    actual="$?"

    if [ "$actual" = "$expected" ]; then
        echo "$input = $expected"
    else
        echo "$input => $expected expected, but got $actual"
        exit 1
    fi
}

try 0 0
try 42 42
try 4 9-8+3
try 243  "100+230-87"
try 9  "4 + 5"
try 40 "(5 + 5)*4"
try 4 "(3+5)/2"
try 15 "5* ( 9 - 6 )"
try 2   "+15 + (4 + -3) * -3 + -10"
try 1  "-4 + -5 + + 10"
try 1  "(-4 + -5) + + 10"
try 1  "-4 + -5 + (+ 10)"
try 4  "+19 + -5 + (- 10)"
echo OK
