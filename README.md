Tripoli
=======

Dependencies
------------

Running 'make test' depends on the [Google C++ Testing Framework](https://code.google.com/p/googletest/). After the usual `./configure` and `make`, the following worked for me:

    sudo cp -a include/gtest /usr/local/include
    sudo cp -a lib/.libs/* /usr/local/lib/

The Makefile assumes OpenFST and the Google C++ Testing Framework can be found under `/usr/local/`.
