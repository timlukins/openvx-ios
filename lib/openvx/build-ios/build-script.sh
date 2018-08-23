#!/usr/bin/env bash
#######################################################################
# Travis/Circle friendly build script
#######################################################################
#
# NOTE: Everything will be assembled in a new folder here called 'InVX'
#
# NOTE: we are checking return codes for failures (xctool should halt anyway, xcodebuild won't!)
#
# NOTE: xctool doesn't work for the tests - so we need xcpretty to generate formatted results.
#   - install with: gem install xcpretty
#
# If you DON'T have your own Apple developer certificates you won't be able to build the final release!
# (the default build settings are for "iOS Developer" and "Automatic" provisioning)
#
# However, encrypted versions of Tim's are shipped in the "certs" folder.
# To use, you will have to first define the following in your environment with the password he gives you!
#
#	export KEY_PASSWORD="XXXXXXX"
#
# Then make sure to run the install-keychain.sh script first (and uninstall-keychain.sh after)
#
# Based on: http://stevenlu.com/posts/2015/08/18/circleci-ios-builds-to-hockeyapp/
# And (for travis): https://www.objc.io/issues/6-build-tools/travis-ci/
#
# NOTE: these same certs are used the "package_ios.sh" script to sign the final release.
#
# WHAT IS CURRENTLY MISSING IS SOME WAY OF PARAMETERISING THE BUILD BASED ON GIT RELEASE TAG!
# Something like: 
# /usr/libexec/PlistBuddy -c "Set :CFBundleVersion $CIRCLE_BUILD_NUM" "yourappname/Info.plist"
#######################################################################

# Build simulator release first (as needed for tests)...
# NOTE: we don't need code signing for this as you won't be redistributing...

xctool build \
  -project OpenVX.xcodeproj \
  -scheme OpenVX \
  -destination 'platform=iOS Simulator,name=iPhone 6' \
  -configuration Release

if [[ $? != 0 ]]; then
  echo "Failed to build SIMULATOR framework!"
  exit 1 
fi

# Run the tests based of this (if they fail this means we won't bother building release).
# Need pipefail to catch exits!
# Currently xctool doesn't bloody work... http://stackoverflow.com/questions/31291474/xctool-can-t-seem-to-see-google-tests
#xctool run-tests \
#  -reporter pretty \
#  -destination 'platform=iOS Simulator,name=iPhone 6,OS=latest' \
#  -sdk iphonesimulator \
#  -project OpenVX.xcodeproj \
#  -configuration Release \
#  -scheme "OpenVX"

set -o pipefail && \
xcodebuild test \
  -project OpenVX.xcodeproj \
  -scheme OpenVX \
  -destination 'platform=iOS Simulator,name=iPhone 6' \
  -configuration Release \
  | xcpretty -r junit --output testresults.xml

if [[ $? != 0 ]]; then
  echo "Failed to run unit tests!"
  exit 1 
fi

# Then build proper release...
# NOTE: we could be explicit here, but the install-keychain will create automatic selected ones.
#   CODE_SIGNING_REQUIRED=YES \
#   CODE_SIGN_IDENTITY="iPhone Developer:  Firstname Lastname (XXXXXXXXXX)" \
#   PROVISIONING_PROFILE="2823AD7F-XXXX-XXXX-XXXX-XXXXXXXXXXX"

set -o pipefail &&  \
xctool build \
  -project OpenVX.xcodeproj \
  -scheme OpenVX \
  -destination 'generic/platform=iOS' \
  -configuration Release 

if [[ $? != 0 ]]; then
  echo "Failed to build DEVICE framework!"
  exit 1 
fi

