#!/bin/bash
set -e
rm -f tests/results/*

echo "Test Embedded data...(expected: 1 warning, no errors)"
./mra tests/test_embedded_data.mra -O tests/results
echo
echo "Test md5 mismatch...(expected: 1 warning, no errors)"
./mra tests/test_md5_mismatch.mra -O tests/results
echo
echo "Test offset and length...(expected: 1 warning, no errors)"
./mra tests/test_offset_length.mra -O tests/results
echo
echo "Test repeat...(expected: no warnings)"
./mra tests/test_repeat.mra -O tests/results
echo
echo "Test interleaving...(expected: no warnings)"
./mra tests/test_interleaved_part.mra -O tests/results
echo
echo "Test endianess...(expected: no warnings)"
./mra tests/test_endianess.mra -O tests/results
echo
echo "Test groups...(expected: 1 error)"
./mra tests/test_groups.mra -O tests/results
echo
echo "Test ARC file...(expected: 1 error)"
./mra tests/test_arc.mra -A -O tests/results
echo
echo "Test Selection by CRC...(expected: no warnings)"
./mra tests/test_select_by_crc.mra -O tests/results
echo
echo "Test Multi zips source...(expected: 1 warning)"
./mra tests/test_multi_zips.mra -O tests/results
echo
echo "Test Patch...(expected: no warnings)"
./mra tests/test_patch.mra -O tests/results
echo
echo "Test file names...(expected: no warnings)"
mkdir -p _test_output_temp
./mra_dir.sh samples/Robotron -AO _test_output_temp > _test_output_temp/log
ls -1 _test_output_temp | grep -E '\.rom|\.arc' | LC_ALL=C sort > tests/results/filenames_test
rm -rf _test_output_temp
echo
echo "Result files (visualize with hexdump -Cv)..."
ls -l tests/results

if [[ `git status --porcelain` ]]; then
  # Changes
  echo "Output files with errors:"
  git status --porcelain
  exit 1
else
  echo "All tests passed"  
fi
