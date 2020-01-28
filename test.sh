#!/bin/sh

rm -f tests/*.rom
echo "Test Embedded data..."
./mra tests/test_embedded_data.mra -z tests
echo "Test md5 mismatch..."
./mra tests/test_md5_mismatch.mra -z tests
echo "Test offset and length..."
./mra tests/test_offset_length.mra -z tests
echo "Test repeat..."
./mra tests/test_repeat.mra -z tests
echo "Test interleaving..."
./mra tests/test_interleaved_part.mra -z tests
echo "Result files (visualize with hexdump -Cv)..."
ls -l tests/*.rom