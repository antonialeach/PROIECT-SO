#!/bin/bash

if [ "$#" -ne 1 ]
then
    echo "Usage: $0 <filename>"
    exit 1
fi

filename="$1"

chmod 444 $filename

if [ ! -f "$filename" ]
then
    echo "File not found: $filename"
    exit 1
fi

contains_keywords=$(grep -P "(malicous|dangerous|corrupted|risk|attack|malware|[^[:ascii:]])" $filename);

line_count=$(wc -l < "$filename")
word_count=$(wc -w < "$filename")
char_count=$(wc -m < "$filename")

if [[ $contains_keywords != "" || $line_count -lt 3 || $word_count -gt 1000 || $char_count -gt 2000 ]]
then
    echo "$filename"
else
    echo "safe"
fi

chmod 000 $filename