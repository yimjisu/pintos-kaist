source ./activate
cd filesys
make
cd build
#pintos -v -k -T 60 -m 20   --fs-disk=10 -p tests/userprog/create-exists:create-exists --swap-disk=4 -- -q   -f run create-exists
make tests/userprog/create-exists.result
# FAIL tests/filesys/base/syn-remove
# FAIL tests/filesys/extended/dir-empty-name
# FAIL tests/filesys/extended/dir-mk-tree
# FAIL tests/filesys/extended/dir-mkdir
# FAIL tests/filesys/extended/dir-open
# FAIL tests/filesys/extended/dir-over-file
# FAIL tests/filesys/extended/dir-rm-cwd
# FAIL tests/filesys/extended/dir-rm-parent
# FAIL tests/filesys/extended/dir-rm-tree
# FAIL tests/filesys/extended/dir-rmdir
# FAIL tests/filesys/extended/dir-under-file
# FAIL tests/filesys/extended/dir-vine
# FAIL tests/filesys/extended/grow-dir-lg
# FAIL tests/filesys/extended/grow-file-size
# FAIL tests/filesys/extended/grow-root-lg