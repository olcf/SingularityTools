* Install the BuilderClient

Three values must be defined to compile and run
* `BUILDER_IP` The IP address of the builder host
* `KEY_FILE` The private key used to SSH into the build host
* `KEY_PASS` The password protecting the private key

```
$ mkdir BUILD && cd BUILD
$ cmake -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=/lustre/sw/... -DBUILDER_IP="123.456.7.8" -DKEY_FILE="/sw/titan/singularity/bloop" -DKEY_PASS="secret" ..
$ make
$ make install
```
