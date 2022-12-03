source ./activate
cd filesys
make
cd build
make tests/filesys/extended/dir-vine.result  
make tests/filesys/extended/syn-rw.result 
make tests/filesys/extended/symlink-file.result 
make tests/filesys/extended/symlink-dir.result 