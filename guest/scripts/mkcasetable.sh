#!/usr/bin/env bash

output=$1
shift 1

cat <<EOF > $output
#include "stitched.h"

EOF

IFS=$'\n' tests=($(sort <<< "$*"))
unset IFS

for test in ${tests[*]}; do
    echo "extern int main_$test(int ac, char **av);" >> $output
done

cat <<EOF >> $output

struct unit_test unit_tests[] = {
EOF

for test in ${tests[*]}; do
    echo "{ .name = \"$test\", .fn = main_$test }," >> $output
done

cat <<EOF >> $output
{ .name = NULL, .fn = NULL },
};
EOF
