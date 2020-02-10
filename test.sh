#!/bin/bash
set -e
rm -f tests/*.rom
echo "Test Embedded data...(expected: 1 warning, no errors)"
./mra tests/test_embedded_data.mra
echo
echo "Test md5 mismatch...(expected: 1 warning, no errors)"
./mra tests/test_md5_mismatch.mra
echo
echo "Test offset and length...(expected: 1 warning, no errors)"
./mra tests/test_offset_length.mra
echo
echo "Test repeat...(expected: no warnings)"
./mra tests/test_repeat.mra
echo
echo "Test interleaving...(expected: no warnings)"
./mra tests/test_interleaved_part.mra
echo
echo "Test endianess...(expected: no warnings)"
./mra tests/test_endianess.mra
echo
echo "Test groups...(expected: 1 error)"
./mra tests/test_groups.mra
echo
echo "Result files (visualize with hexdump -Cv)..."
ls -l tests/*.rom

if [[ `git status --porcelain | grep .rom` ]]; then
  # Changes
  echo "ROM files changed:"
  git status --porcelain | grep .rom
  exit 1
fi
