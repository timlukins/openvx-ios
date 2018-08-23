# InVX for MacOS

This build is entirely commandline driven and supported by CMake.

Thus it can be automated via the same calls in a top-level .travis.yaml

## Environment setup

MacOS homebrew commands:

Notice the accompanying vxtest.cpp file in this folder which implements the necessary `../test' library methods for running on the command line. 


## Build command

You should not perform this build, or tests, if you have already installed OpenVX (e.g. via the homebrew tap). Remove any other install to avoid confusion!

**In this folder!**

	> cmake -DAPPLE=1 -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=/path/to/install/to -DBUILD_X64=1 .. 
	> make

## Test command

NOTE: at the moment the built libraries are not installed. Recommend that you set a temporary path in the above INSTALL_PREFIX_ so that you can do:

	> make install

And then control dynamic loading and consequent performance increases use:

	> DYLD_LIBRARY_PATH="/path/to/install/to"

(by then temporarily moving the .dylib build artefacts or changing the path.)

Then using ctest:

	> ctest 

Or, to see individual tests:

	> cd ./test
	> ./testinvx

## Package command

MacOS run this script to wrap the static libs into a framework... TODO


