source ./activate
cd userprog
make clean
make
cd build
#pintos --fs-disk=10 -p tests/userprog/args-single:args-single -- -q -f run 'args-single onearg'
make tests/filesys/base/syn-read.result
make tests/filesys/base/syn-write.result
make tests/userprog/dup2/dup2-complex.result