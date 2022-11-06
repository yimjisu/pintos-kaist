source ./activate
cd vm
make
cd build
pintos -v -k -T 60 -m 20   --fs-disk=10 -p tests/vm/pt-write-code2:pt-write-code2 -p ../../tests/vm/sample.txt:sample.txt --swap-disk=4 -- -q   -f run pt-write-code2 < /dev/null 2> tests/vm/pt-write-code2.errors > tests/vm/pt-write-code2.output
perl -I../.. ../../tests/vm/pt-write-code2.ck tests/vm/pt-write-code2 tests/vm/pt-write-code2.result
pintos -v -k -T 600 -m 20   --fs-disk=10 -p tests/vm/page-merge-par:page-merge-par -p tests/vm/child-sort:child-sort --swap-disk=10 -- -q   -f run page-merge-par < /dev/null 2> tests/vm/page-merge-par.errors > tests/vm/page-merge-par.output
perl -I../.. ../../tests/vm/page-merge-par.ck tests/vm/page-merge-par tests/vm/page-merge-par.result
