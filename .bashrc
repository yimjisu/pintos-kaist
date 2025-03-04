source ./activate
# cd vm
# make
# cd build
# pintos -v -k -T 180 -m 8   --fs-disk=10 -p tests/vm/swap-file:swap-file -p ../../tests/vm/large.txt:large.txt --swap-disk=10 -- -q   -f run swap-file

cd vm
make
cd build

make tests/userprog/fork-multiple.result
make tests/vm/pt-grow-stack.result
make tests/vm/pt-grow-bad.result
make tests/vm/pt-big-stk-obj.result
make tests/vm/pt-bad-addr.result
make tests/vm/pt-bad-read.result
make tests/vm/pt-write-code.result
make tests/vm/pt-write-code2.result
make tests/vm/pt-grow-stk-sc.result
make tests/vm/page-linear.result
make tests/vm/page-parallel.result
make tests/vm/page-merge-seq.result
make tests/vm/page-merge-par.result
make tests/vm/page-merge-stk.result
make tests/vm/page-merge-mm.result
make tests/vm/page-shuffle.result
make tests/vm/mmap-read.result
make tests/vm/mmap-close.result
make tests/vm/mmap-unmap.result
make tests/vm/mmap-overlap.result
make tests/vm/mmap-twice.result
make tests/vm/mmap-write.result
make tests/vm/mmap-ro.result
make tests/vm/mmap-exit.result
make tests/vm/mmap-shuffle.result
make tests/vm/mmap-bad-fd.result
make tests/vm/mmap-clean.result
make tests/vm/mmap-inherit.result
make tests/vm/mmap-misalign.result
make tests/vm/mmap-null.result
make tests/vm/mmap-over-code.result
make tests/vm/mmap-over-data.result
make tests/vm/mmap-over-stk.result
make tests/vm/mmap-remove.result
make tests/vm/mmap-zero.result
make tests/vm/mmap-bad-fd2.result
make tests/vm/mmap-bad-fd3.result
make tests/vm/mmap-zero-len.result
make tests/vm/mmap-off.result
make tests/filesys/base/syn-read.result
make tests/vm/mmap-bad-off.result
make tests/vm/mmap-kernel.result
make tests/vm/lazy-file.result
make tests/vm/lazy-anon.result
make tests/vm/swap-file.result
make tests/vm/swap-anon.result
make tests/vm/swap-iter.result
make tests/vm/swap-fork
make tests/vm/cow/cow-simple
