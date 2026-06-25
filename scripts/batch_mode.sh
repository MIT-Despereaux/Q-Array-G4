#!/bin/bash

current_macro_path=$1

# Verify the file exists before running sed
if [[ ! -f "$current_macro_path" ]]; then
    echo "Error: File '$current_macro_path' not found."
    exit 1
fi

if [[ "$OSTYPE" == "darwin"* ]]; then
    # macOS format
    sed -i '' \
        -e 's|^#/vis/|/vis/|' \
        -e 't' \
        -e 's|^\(/vis/\)|#\1|' \
        "$current_macro_path"
else
    # Linux/GNU format
    sed -i \
        -e 's|^#/vis/|/vis/|' \
        -e 't' \
        -e 's|^\(/vis/\)|#\1|' \
        "$current_macro_path"
fi