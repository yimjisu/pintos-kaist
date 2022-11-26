source ./activate
cd filesys
make
cd build
pintos -v -k -T 60 -m 20   --fs-disk=10 -p tests/userprog/create-exists:create-exists --swap-disk=4 -- -q   -f run create-exists

# make tests/userprog/create-exists.result
# make tests/userprog/open-normal.result
# make tests/userprog/open-boundary.result
# make tests/userprog/open-twice.result
# make tests/userprog/close-normal.result
# make tests/userprog/close-twice.result
# make tests/userprog/read-normal.result
# make tests/userprog/read-bad-ptr.result
# make tests/userprog/read-boundary.result
# make tests/userprog/read-zero.result
# make tests/userprog/write-normal.result
# make tests/userprog/write-bad-ptr.result
# make tests/userprog/write-boundary.result
# make tests/userprog/write-zero.result
# make tests/userprog/fork-read.result
# make tests/userprog/fork-close.result
# make tests/userprog/exec-once.result
# make tests/userprog/exec-arg.result
# make tests/userprog/exec-boundary.result
# make tests/userprog/exec-read.result
# make tests/userprog/wait-simple.result
# make tests/userprog/wait-twice.result
# make tests/userprog/wait-killed.result
# make tests/userprog/multi-child-fd.result
# make tests/userprog/rox-child.result
# make tests/userprog/rox-multichild