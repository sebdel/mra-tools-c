#!/bin/bash
for f in *.mra
do
    echo "Processing $f file..."
    ../mra-tools-c/mra -A -z ../roms "$f"
done
echo "Generated ROMS:"
ls -la *.rom
