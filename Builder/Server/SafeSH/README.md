# SafeSH
SafeSH provides a safe way to control what commands are run over SSH for a given authorized key.

# Building
SafeSH requires C++14 and should buildable with recent versions of GCC or Clang. Boost and CMake are also required.

```
$ mkdir build && cd build
$ cmake -DCMAKE_BUILD_TYPE=Release ..
$ make
$ make install
```

# Usage
Set the `command` option to the desired key in `authorized_keys`. `SafeSH` will then be the command invoked when the key is used.


```
$ cat $HOME/.ssh/authorized_keys
command="/usr/local/bin/SafeSH "$SSH_ORIGINAL_COMMAND"" {KEY_TYPE} {KEY}
```