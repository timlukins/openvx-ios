InVX for iOS 
============

TL;DR

On an up-to-date Mac run the ./build-script.sh

NOTE HOWEVER - this will only work if you have your own iOS developer code signing and provisioning profile!

Otherwise:
	- You will at least be able to do the simulator build and tests (see the first 2 commands of build-script).

	- You can run the install-keychain.sh to install (Tims) certificates. Run uinstall-keychain after. NB. You must set the environment KEY_PASSWORD!

---

This build is controlled by a XCode project, which was originally generated via CMake and then had the unit tests added to it. See technical details at the end.

## Environment setup

The project is geared to be run via 'xctool" so:

	> brew update
	> brew install xctool

If you are not using homebrew, then install xctool as follows:

1. Clone xctool:
	> git clone https://github.com/facebook/xctool.git
2. cd to xctool folder.
3. Build using:
	> ./xctool.sh -help
4. Then, add the installation folder to your PATH environment variable in .bashrc, i.e., add the following lines at the end:
	export XCTOOL=/path/to/xctool/build/ff3dcdb/7B91b/Products/Release
	export PATH=$PATH:${XCTOOL}

Notice the accompanying OpenVXTest/OpenVXTests.mm file in this folder which implements the necessary `../test' library methods for running from an iOS test bundle. 

## Build command

**In this folder!**

Using xctool you need to specify the scheme and destination (as well as the Release configuration).

Build for both simulator and actual iOS.

The results will go (via the package_ios script that is invoked) into a "InVX" artefact folder.

If you ever need to create a debug version, change the configuration (but the tests might fail to find the framework).

	> xctool build -project OpenVX.xcodeproj -scheme OpenVX -destination 'platform=iOS Simulator,name=iPhone 6' -configuration Release
	> xctool build -project OpenVX.xcodeproj -scheme OpenVX -destination 'generic/platform=iOS' -configuration Release
  

## Test command

We use googletest - but to make it play nicely with XCode there is a bit more work. See here:  https://github.com/mattstevens/xcode-googletest

We have added a OpenVXTest bundle target to the XCode project manually to leverage the same googletests.

To do this we have to build googletest as a framework itself (for simulator). This was done and is found in ../test/lib/gtest-1.7.0/xcode

The code can be found in OpenVXTest.mm - including implementations of the bundle methods

NOTICE: it is extremely important for the test bundle to run the same way the App would do - i.e. with the other .frameworks included for dynamic loading (see the Copy Files build phase).

	> xctool run-tests -project OpenVX.xcodeproj -scheme OpenVX -sdk iphonesimulator -configuration Release

NOTE: currently this doesn't work - but this does:

	> xcodebuild test -project OpenVX.xcodeproj -scheme OpenVX -destination 'platform=iOS Simulator,name=iPhone 6' -configuration Release

## Archive command

The ./package_ios.sh script is run as part of the build and generates the InVX folder, in which there should now be two subfolders - Release-iphone and Release-iphonesimulator.

This folder can be ziped and uploaded as a release.

## TODO:

This could all still be driven by CMake?

https://github.com/ruslo/sugar/blob/master/examples/03-ios-gtest/CMakeLists.txt

## Tecnical Details
 
Here are the rough steps that were follwed to use CMake to generate the original project and then modify it by hand... 


1) In project *root* directory - Generate from CMake project via: cmake -DCMAKE_TOOLCHAIN_FILE=./cmake_utils/iOS.cmake -DIOS_PLATFORM=OS -DVERSION_MAJOR=1 -DVERSION_MINOR=0 -DVERSION_PATCH=7 -DCMAKE_BUILD_TYPE=Release -GXcode .

2) This will generate extra files (remove with git clean -f -d ) but keep the OpenVX.xcodeproj and move it to this directory.

3) Open the XCode project and remove the ALL_BUILD, ZERO_CHECK, RUN_TESTS, and install folders.

4) In each remaining folder, notice that all the references to files are red (i.e. path wrong). 

5) Remove all CMakeLists.txt file references from all folders/targets. Also the OpenVX-CMakeForceLinker.cxx file under OpenVX.

6) This should result in a neat, orderly breakdown of all the project files (still) red in respective Folders/target/Soure Files...

7) Under the project nav - delete the targets and also delete any build phase that has a CMake PostBuild Rules (e.g. openvx-debug).

8) Under Product->Schemes->Manage Schemes, again delete the ALL_BUILD, ZERO_CHECK etc ones and make sure the remaining OpenVX one is shared (crucial for xctool).

9) Close the XCode project and update the paths in the project file by the following:

		Use VIM to substitute a relative path in front of every file included, with :%s/path = \(\w\+\)/path = ..\/\1/g	

		And for all the build paths with :%s/\/Users\/tim\/Code\/mwiv\/invx/./g
		
		And all the header include paths with :%s/\s+\/Users\/tim\/Code\/mwiv\/invx/../g

		NOTE: for this replace the absolute path with the one generated for your home account. It means we still get an out of code build, but header search paths and files are correctly referenced. 

10) If you then open the XCode project you should find all the red files have turned black - indicating they are referenced!

11) You should retain the autogenerated Info.plist file and commit it here. You'll also need to edit the project.pbxproj to point at it:

		INFOPLIST_FILE = ./Info.plist;	

	The effect of this will be apparent - as the OpenVX.framework "Info" tab will then work!

12) At this point, you should now be able to use xctool as per the first build step below. i.e. 

		xcodebuild -target OpenVX -configuration Release
		xctool build -project OpenVX.xcodeproj -scheme OpenVX -configuration Release


) To add the test - create a new target in XCode of type iOS->Other->Cocoa Touch Testing Bundle and call it "OpenVX Tests". The defaults, including target to be tested bein g the OpenVX.framework - should be fine. 

) Because we have Autocreate schemes turned on this will create an appropriate scheme - but we want to add it to the test phase of the OpenVX scheme, so edit this scheme to point at the tests!

) The "OpenVX Tests" target folder has then been addded automatically to this directory - with its own OpenVXtests.m source file. Renamed this file to OpenVXTests.mm (since we are to integrate it with a 

) Moved the contents of the CMake created "Test" folder to under the "OpenVXTests" target one (this doesn't change 

) Included in the OpenVXtest.mm this stuff:

) Built (using the ../test/lib/gtest-1.7.0/xcode folder contents) a working STATIC framework version for iphonesimulator and ios (this involved manually going through the entire XCode project for gtest and updating the target to be iOS before then. Checked in these static frameworks under the the gtest-1.7.0 folder. 

) To ensure that the package script runs before the tests added a new aggregate called "Package" that actually runs the package_ios script (and is a dependency of the tests and depends on the libs). 

) For the tests, ensured that when targetting either iphonesimulator or ios - different paths were used to resolve the static gtest framework and the (assembled by the aggregate target 

) Added a Copy Bundle Resources phase to the OpenVXTests and added (from project hierarchy where it was also added) the *.pgm test/data files...



