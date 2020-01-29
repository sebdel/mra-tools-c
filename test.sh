#!/bin/sh
red=`tput setaf 1`
green=`tput setaf 2`
yellow=`tput setaf 3`
bold=`tput bold`
reset=`tput sgr0`
rm -f tests/*.rom
echo "${bold}Test Embedded data...(expected: ${yellow}1 warning${reset}${bold}, no errors)${reset}"
./mra tests/test_embedded_data.mra -z tests
echo
echo "${bold}Test md5 mismatch...(expected: ${yellow}1 warning${reset}${bold}, no errors)${reset}"
./mra tests/test_md5_mismatch.mra -z tests
echo
echo "${bold}Test offset and length...(expected: ${yellow}1 warning${reset}${bold}, no errors)${reset}"
./mra tests/test_offset_length.mra -z tests
echo
echo "${bold}Test repeat...(expected: no warnings)${reset}"
./mra tests/test_repeat.mra -z tests
echo
echo "${bold}Test interleaving...(expected: no warnings)${reset}"
./mra tests/test_interleaved_part.mra -z tests
echo
echo "${bold}Result files (visualize with hexdump -Cv)...${reset}"
tput setaf 2
ls -l tests/*.rom