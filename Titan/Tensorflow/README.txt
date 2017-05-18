# Tensorflow Container
Provides Tensorflow/1.0.1 with Python2/3 interface. Additional Keras support provided.

sudo singularity create --force --size 15000 tensorflow_x86.img
sudo -E singularity bootstrap tensorflow_x86.img tensorflow.def
