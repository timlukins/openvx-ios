#!/usr/bin/env bash
# This script assumes that ImageMagick is installed and the convert command is accessible via the $PATH variable

# Ensure that one argument has been passed in.
if [ ! "$#" -eq 1 ]
then
	echo -e "This script requires one argument.\\ne.g. iOS_icon_maker.sh app_icon.png"
	exit 1
fi

# Assign the argument to the path variable so it is easier to follow throughout the script.
path=$1

# Ensure that the path points to a valid file.
if [ ! -f "$path" ]
then
	echo "Path must point to a valid file."
	exit 1
fi

# This function takes in the dimension of the icon (it assumes the icon is a square) and the name of the file to save the icon to.
function createIconImage()
{
	iconDimension=$1
	iconName=$2

	convert "$path" -resize ${iconDimension}x${iconDimension}^ -gravity center -extent ${iconDimension}x${iconDimension} $iconName
}

# Create all the suggested icons for the Android platform to ensure the best appearance.

createIconImage 36 ic_launcher-ldpi.png
createIconImage 48 ic_launcher-mdpi.png
createIconImage 64 ic_launcher-tvdpi.png
createIconImage 72 ic_launcher-hdpi.png
createIconImage 96 ic_launcher-xhdpi.png
createIconImage 128 icon.png
createIconImage 144 ic_launcher-xxhdpi.png
createIconImage 192 ic_launcher-xxxhdpi.png
