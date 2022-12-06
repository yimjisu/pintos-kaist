source ./activate
# cd vm
# make
# cd build
# pintos -v -k -T 180 -m 8   --fs-disk=10 -p tests/vm/swap-file:swap-file -p ../../tests/vm/large.txt:large.txt --swap-disk=10 -- -q   -f run swap-file

cd filesys
make
cd build
# rm -f tmp.dsk
# pintos-mkdisk tmp.dsk 2
# pintos -v -k -T 60 -m 20   --fs-disk=tmp.dsk -p tests/filesys/extended/syn-rw:syn-rw -p tests/filesys/extended/tar:tar -p tests/filesys/extended/child-syn-rw:child-syn-rw --swap-disk=4 -- -q   -f run syn-rw

make tests/filesys/extended/syn-rw.result 
# make tests/filesys/extended/dir-vine.result 
# make tests/vm/page-merge-stk.result 
# make tests/vm/mmap-write.result 
# make tests/vm/mmap-off.result 
# make tests/filesys/extended/syn-rw-persistenc.result 
# make tests/filesys/extended/dir-vine-persistence