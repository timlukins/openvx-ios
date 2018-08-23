#!/usr/bin/env bash
#######################################################################
# Script to uninstall a temporary keychain and remove provisioning profile.
#######################################################################

security delete-keychain ios-build.keychain
rm -f ~/Library/MobileDevice/Provisioning\ Profiles/*
