source ./activate
cd vm
make
cd build
pintos -v -k -T 60 -m 20   --fs-disk=10 -p tests/userprog/args-none:args-none --swap-disk=4 -- -q   -f run args-none 