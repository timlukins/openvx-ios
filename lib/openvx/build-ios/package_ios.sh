#!/usr/bin/env bash
#######################################################################
#
# This script takes the build output for iOS and "officially" packages it.
#
# It's designed to run as a separate "Run Script" phase in Xcode.
#
# Hence why it picks up the configuration and platform name below...
#
#######################################################################

set -e # Fail on error!

# ALL IMPORTANT INTERFACE WITH XCODE HERE!
export BUILDDIR="InVX/${CONFIGURATION}${EFFECTIVE_PLATFORM_NAME}"

echo ">>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>> Starting packaging for $BUILDDIR..."

rm -rf $BUILDDIR #*.framework # Going to clear these out first!

export PRODUCT_NAME="OpenVX"
export PRODUCT_LIB_NAME="openvx"
export PRODUCT_CONFIG="${CONFIGURATION}" #"Release" # Debug or Release - must match invocation via CMake
export PRODUCT_TARGET="${EFFECTIVE_PLATFORM_NAME}" #"-iphonesimulator" # This is as a result of CMake -DIOS_PLATFORM=OS or -iphoneos or -iphonesimulator NOTE: leading dash!

export STATIC_LIB="lib${PRODUCT_LIB_NAME}-lib.a"
export MODEL_LIB="lib${PRODUCT_LIB_NAME}-c_model.dylib"
export EXTRAS_LIB="lib${PRODUCT_LIB_NAME}-extras.dylib"
export INVX_LIB="lib${PRODUCT_LIB_NAME}-accelerate.dylib"

# Make the *UNIVERSAL* framework folder (with resources)... 
export FRAMEWORK_LOCN="${BUILDDIR}/${PRODUCT_NAME}.framework"
export MODEL_FRAMEWORK_LOCN="${BUILDDIR}/libopenvx-cmodel.framework" # NOTE: App store don't like _underscore_
export EXTRAS_FRAMEWORK_LOCN="${BUILDDIR}/${EXTRAS_LIB%.*}.framework"
export INVX_FRAMEWORK_LOCN="${BUILDDIR}/libinvx.framework" # NOTE: hardcoded for particular name

mkdir -p ${FRAMEWORK_LOCN}
mkdir -p "${FRAMEWORK_LOCN}/Versions/A/Resources"
mkdir -p "${FRAMEWORK_LOCN}/Versions/A/Headers"
mkdir -p ${MODEL_FRAMEWORK_LOCN}
mkdir -p ${EXTRAS_FRAMEWORK_LOCN}
mkdir -p ${INVX_FRAMEWORK_LOCN}

export STATIC_DEST="${FRAMEWORK_LOCN}/Versions/A/${PRODUCT_NAME}"
export MODEL_DEST="${MODEL_FRAMEWORK_LOCN}/libopenvx-cmodel" # Hardcoded for renaming without _underscore_
export EXTRAS_DEST="${EXTRAS_FRAMEWORK_LOCN}/${EXTRAS_LIB%.*}"
#export INVX_DEST="${INVX_FRAMEWORK_LOCN}/${INVX_LIB%.*}"
export INVX_DEST="${INVX_FRAMEWORK_LOCN}/libinvx" # NOTE: also hard coded for particular name

# Results in: 
# iphoneos/Release-iphoneos/openvx.framework
# iphoneos/sample/framework/Release-iphoneos/libopenvx-lib.a
# iphoneos/sample/targets/c_model/Release-iphoneos/libopenvx-c_model.dylib
# iphoneos/libraries/extras/libopenvx-extras.dylib

export STATIC_LOCN="./sample/framework/${PRODUCT_CONFIG}${PRODUCT_TARGET}/${STATIC_LIB}"
export MODEL_LOCN="./sample/targets/c_model/${PRODUCT_CONFIG}${PRODUCT_TARGET}/${MODEL_LIB}"
export EXTRAS_LOCN="./libraries/extras/${PRODUCT_CONFIG}${PRODUCT_TARGET}/${EXTRAS_LIB}"
export INVX_LOCN="./sample/targets/accelerate/${PRODUCT_CONFIG}${PRODUCT_TARGET}/${INVX_LIB}"

if [ ! -f ${STATIC_DEST} ]
then
	cp ${STATIC_LOCN} ${STATIC_DEST}
else
	xcrun lipo -create "${STATIC_LOCN}" "${STATIC_DEST}" -output "${STATIC_DEST}"
fi	
if [ ! -f ${MODEL_DEST} ]
then
	cp ${MODEL_LOCN} ${MODEL_DEST}
else
	xcrun lipo -create "${MODEL_LOCN}" "${MODEL_DEST}" -output "${MODEL_DEST}"
fi	
if [ ! -f ${EXTRAS_DEST} ]
then
	cp ${EXTRAS_LOCN} ${EXTRAS_DEST}
else
	xcrun lipo -create "${EXTRAS_LOCN}" "${EXTRAS_DEST}" -output "${EXTRAS_DEST}"
fi
if [ ! -f ${INVX_DEST} ]
then
	cp ${INVX_LOCN} ${INVX_DEST}
else
	xcrun lipo -create "${INVX_LOCN}" "${INVX_DEST}" -output "${INVX_DEST}"
fi

# Copy info.plist and headers...

if [ ! -f "${FRAMEWORK_LOCN}/Versions/A/Resources/Info.plist" ] # Will just copy first one 
then
	cp "./${PRODUCT_CONFIG}${PRODUCT_TARGET}/${PRODUCT_NAME}.framework/Info.plist" "${FRAMEWORK_LOCN}/Versions/A/Resources"
fi

if [ ! -f "${FRAMEWORK_LOCN}/Versions/A/Headers" ]  
then
	cp -r "./${PRODUCT_CONFIG}${PRODUCT_TARGET}/${PRODUCT_NAME}.framework/Versions/A/Headers" "${FRAMEWORK_LOCN}/Versions/A/"
fi

# Create the required symlinks within the OpenVX framework...
ln -sfh A "${FRAMEWORK_LOCN}/Versions/Current"
ln -sfh "Versions/Current/Headers" "${FRAMEWORK_LOCN}/Headers"
ln -sfh "Versions/Current/${PRODUCT_NAME}" "${FRAMEWORK_LOCN}/${PRODUCT_NAME}" 

# Rename the .dylib's so the ID is the same name as the framework...

xcrun install_name_tool -id "${MODEL_LIB%.*}" "${MODEL_DEST}" 
xcrun install_name_tool -id "${EXTRAS_LIB%.*}" "${EXTRAS_DEST}" 

# Copy the .plist into the dylib frameworks (and modify slightly)
cp "${FRAMEWORK_LOCN}/Versions/A/Resources/Info.plist" "${MODEL_FRAMEWORK_LOCN}/." 
/usr/libexec/PlistBuddy -c "Set :CFBundleIdentifier com.machineswithvision.libopenvx-cmodel" "${MODEL_FRAMEWORK_LOCN}/Info.plist" # NB> hard coded to rename
/usr/libexec/PlistBuddy -c "Set :CFBundleExecutable libopenvx-cmodel" "${MODEL_FRAMEWORK_LOCN}/Info.plist"
cp "${FRAMEWORK_LOCN}/Versions/A/Resources/Info.plist" "${EXTRAS_FRAMEWORK_LOCN}/." 
/usr/libexec/PlistBuddy -c "Set :CFBundleIdentifier com.machineswithvision.${EXTRAS_LIB%.*}" "${EXTRAS_FRAMEWORK_LOCN}/Info.plist"
/usr/libexec/PlistBuddy -c "Set :CFBundleExecutable ${EXTRAS_LIB%.*}" "${EXTRAS_FRAMEWORK_LOCN}/Info.plist"
cp "${FRAMEWORK_LOCN}/Versions/A/Resources/Info.plist" "${INVX_FRAMEWORK_LOCN}/."
/usr/libexec/PlistBuddy -c "Set :CFBundleIdentifier com.machineswithvision.libinvx" "${INVX_FRAMEWORK_LOCN}/Info.plist" # NB> hard coded to rename
/usr/libexec/PlistBuddy -c "Set :CFBundleExecutable libinvx" "${INVX_FRAMEWORK_LOCN}/Info.plist" # NB> hard coded to rename
	
# Copy the LICENSE
cp ../LICENSE "${FRAMEWORK_LOCN}/Versions/A/Resources/LICENSE.txt"
cp ../LICENSE "${MODEL_FRAMEWORK_LOCN}/LICENSE.txt"
cp ../LICENSE "${EXTRAS_FRAMEWORK_LOCN}/LICENSE.txt"
cp ../LICENSE "${INVX_FRAMEWORK_LOCN}/LICENSE.txt"

# Resign the framework...
#codesign --sign "iPhone Distribution" --force --verbose=4 "${FRAMEWORK_LOCN}"
#codesign --sign "iPhone Distribution" --force --verbose=4 "${MODEL_FRAMEWORK_LOCN}"
#codesign --sign "iPhone Distribution" --force --verbose=4 "${EXTRAS_FRAMEWORK_LOCN}"
#codesign --sign "iPhone Distribution" --force --verbose=4 "${INVX_FRAMEWORK_LOCN}"

# N.B. You can check the out libs the arch's with otool as follows...

echo ">>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>> Package finished!"

echo "Here are the architectures supported:"

echo `xcrun lipo -info ${STATIC_DEST}`  
echo `xcrun lipo -info ${MODEL_DEST}`  
echo `xcrun lipo -info ${EXTRAS_DEST}`  
echo `xcrun lipo -info ${INVX_DEST}`  

