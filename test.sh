#!/bin/bash
set -e
rm -f tests/*.rom
echo "Test Embedded data...(expected: 1 warning, no errors)"
./mra tests/test_embedded_data.mra -z tests
echo
echo "Test md5 mismatch...(expected: 1 warning, no errors)"
./mra tests/test_md5_mismatch.mra -z tests
echo
echo "Test offset and length...(expected: 1 warning, no errors)"
./mra tests/test_offset_length.mra -z tests
echo
echo "Test repeat...(expected: no warnings)"
./mra tests/test_repeat.mra -z tests
echo
echo "Test interleaving...(expected: no warnings)"
./mra tests/test_interleaved_part.mra -z tests
echo
echo "Result files (visualize with hexdump -Cv)..."
ls -l tests/*.rom

if [[ `git status --porcelain | grep .rom` ]]; then
  # Changes
  echo "ROM files changed:"
  git status --porcelain | grep .rom
  exit 1
fi
