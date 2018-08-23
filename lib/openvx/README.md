
                                          
    _|_|_|            _|      _|  _|      _|  
      _|    _|_|_|    _|      _|    _|  _|    
      _|    _|    _|  _|      _|      _|      	
      _|    _|    _|    _|  _|      _|  _|    
    _|_|_|  _|    _|      _|      _|      _|  


          Machines With Vision, 2015

A heavily modified version of the OpenVX 1.0.1 sample implementation to support builds for:
	
* Android .so (via gradle)
* iOS 8+ .framework (via xcodebuild) 
* MacOS .dylib (via CMake)

Including the following accelerated target backends:

* `./sample/target/accelerate` using Accelerate for iOS and MacOS.
* `./sample/target/???` using Renderscript for Android.
* `./sample/target/???` using OpenGL shaders for ??

Plus, extensive unit and correctness testing via the following folders:

* `./test` - quick "does it work" and performance
* `./conformance` - the khronos conformance test suite (for 1.0.1)

THE AUTOMATED BUILD AT THE MOMENT ONLY DOES THE IOS VERSION

For build instructions - either look at the circle.yml file or manually look at the README.md & build-script.sh in the respective target build folder:

* `./build-macos`
* `./build-ios`
* `./build-android`

Generally, each build-* folder is responsible for their individual artifacts.  The resulting artifacts are combined into a single *.zip to form the composite release.

TODO AN OFFICIAL RELEASE:

1) First, merge onto development - this will trigger a candiate build.
2) Fetch the resulting artifact (InVX-VERSION-BUILD_NUMBER.zip) from CircleCI.
3) Run the conformance tests using this build.
4) If suitable, create a new vX.Y.Z release tag.
5) Promote the canditate build number to this (TODO: add how) 
=======
The resulting artefacts are combined into a release.

Website
=======

The actual *public* (yes _public_) website is in the "website" folder.

You can push this to the special gh-pages branch with:

	git subtree push --prefix website origin gh-pages

Note: the build artifacts are copied to the website/download/sdk folder as a zip.

This is where the CocoaPods resolves the source directive to. E.g. spec.source = { :http => 'http://invx.io/download/sdk/invx.zip' }

	pod repo add mwiv-specs git@github.com:machineswithvision/Specs.git

Good example for what we are trying to do is the Parse one:

https://github.com/CocoaPods/Specs/blob/master/Specs/Parse/1.7.4/Parse.podspec.json
