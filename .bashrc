source ./activate
# cd vm
# make
# cd build
# pintos -v -k -T 180 -m 8   --fs-disk=10 -p tests/vm/swap-file:swap-file -p ../../tests/vm/large.txt:large.txt --swap-disk=10 -- -q   -f run swap-file

cd vm
make
cd build
#pintos -v -k -T 60 -m 20   --fs-disk=10 -p tests/vm/mmap-write:mmap-write --swap-disk=4 -- -q   -f run mmap-write
make tests/vm/page-merge-par.result 
make tests/vm/page-merge-stk.result 
make tests/vm/page-merge-mm.result 
make tests/vm/mmap-write.result 
make tests/vm/mmap-exit.result 
make tests/vm/mmap-off.result 
make tests/vm/mmap-kernel.result 
