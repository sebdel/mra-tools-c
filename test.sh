#!/bin/bash
set -e
rm -f tests/*.ROM
rm -f tests/*.arc
echo "Test Embedded data...(expected: 1 warning, no errors)"
./mra tests/test_embedded_data.mra -O tests
echo
echo "Test md5 mismatch...(expected: 1 warning, no errors)"
./mra tests/test_md5_mismatch.mra -O tests
echo
echo "Test offset and length...(expected: 1 warning, no errors)"
./mra tests/test_offset_length.mra -O tests
echo
echo "Test repeat...(expected: no warnings)"
./mra tests/test_repeat.mra -O tests
echo
echo "Test interleaving...(expected: no warnings)"
./mra tests/test_interleaved_part.mra -O tests
echo
echo "Test endianess...(expected: no warnings)"
./mra tests/test_endianess.mra -O tests
echo
echo "Test groups...(expected: 1 error)"
./mra tests/test_groups.mra -O tests
echo
echo "Test ARC file...(expected: 1 error)"
./mra tests/test_arc.mra -A -O tests
echo
echo "Test Selection by CRC...(expected: no warnings)"
./mra tests/test_select_by_crc.mra -O tests
echo
echo "Test Multi zips source...(expected: 1 warning)"
./mra tests/test_multi_zips.mra -O tests
echo
echo "Test Patch...(expected: no warnings)"
./mra tests/test_patch.mra -O tests
echo
echo "Result files (visualize with hexdump -Cv)..."
ls -l tests/*.ROM

if [[ `git status --porcelain | grep -E '\.ROM|\.arc'` ]]; then
  # Changes
  echo "Output files with errors:"
  git status --porcelain | grep -E '\.ROM|\.arc'
  exit 1
fi
