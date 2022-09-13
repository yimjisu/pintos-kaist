source ./activate
cd threads
make
cd build
pintos -v -k -T 480 -m 20   -- -q  -mlfqs run mlfqs-nice-10 < /dev/null 2> tests/threads/mlfqs/mlfqs-nice-10.errors > tests/threads/mlfqs/mlfqs-nice-10.output
perl -I../.. ../../tests/threads/mlfqs/mlfqs-nice-10.ck tests/threads/mlfqs/mlfqs-nice-10 tests/threads/mlfqs/mlfqs-nice-10.result
