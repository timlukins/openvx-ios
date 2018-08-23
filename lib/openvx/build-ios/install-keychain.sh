#!/usr/bin/env bash
#######################################################################
# Script to install a temporary keychain and provisioning profile for building.
#######################################################################
# 
# This is designed to be run in a build environment with no prior Apple
# iPhone Developer or Distribution certificates.
# 
# So, instead Tim has exported his (expires October 2016 at time of writing).
# 
# DO NOT RUN IF YOU HAVE YOUR OWN APPLE DEVELOPER PROFILES AND CERTS!
#
# To use you must provide the password given to you by Tim:
#
#	export KEY_PASSWORD="XXXXXXX"
# 
# This can also be used on Circle/Travis to set up a build env.
#
# WHEN FINISHED REMOVE WITH uninstall-keychain.sh
#
#######################################################################

# Temporary (don't care) keychain password
KEYCHAIN_PASSWORD=circleci

security create-keychain -p $KEYCHAIN_PASSWORD ios-build.keychain
security import ./certs/apple.cer -k ~/Library/Keychains/ios-build.keychain -T /usr/bin/codesign

security import ./certs/dev.cer -k ~/Library/Keychains/ios-build.keychain -T /usr/bin/codesign
security import ./certs/dist.cer -k ~/Library/Keychains/ios-build.keychain -T /usr/bin/codesign
security import ./certs/dev.p12 -k ~/Library/Keychains/ios-build.keychain -P $KEY_PASSWORD -T /usr/bin/codesign
security import ./certs/dist.p12 -k ~/Library/Keychains/ios-build.keychain -P $KEY_PASSWORD -T /usr/bin/codesign

security list-keychain -s ~/Library/Keychains/ios-build.keychain
security unlock-keychain -p $KEYCHAIN_PASSWORD ~/Library/Keychains/ios-build.keychain

mkdir -p ~/Library/MobileDevice/Provisioning\ Profiles
cp ./certs/Tims.mobileprovision ~/Library/MobileDevice/Provisioning\ Profiles/
