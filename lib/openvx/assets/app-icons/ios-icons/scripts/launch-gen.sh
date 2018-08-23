#!/bin/bash
# @Author: CodePug, C. 2014
# usage:  generate-iOS-app-launch.sh sourceImage-1024x1024.png

SOURCE_ICON="$1"
BG_COLOR="#ffffff"
CMD="../${SOURCE_ICON} -resize 100x100^ -gravity center -borderColor ${BG_COLOR} -border 1200x1200"

mkdir -p launchimg 
cd launchimg 

echo -n Generating Images:
convert ${CMD} -crop 1242x2208+0+0 +repage ios8P_PH55RetHD.png
convert ${CMD} -crop 750x1334+0+0 +repage ios8P_PH47RetHD.png
convert ${CMD} -crop 2208x1242+0+0 +repage ios8L_PH55RetHD.png
convert ${CMD} -crop 768x1024+0+0 +repage ios7P_PADx1.png
convert ${CMD} -crop 1536x2048+0+0 +repage ios7P_PADx2.png
convert ${CMD} -crop 1024x768+0+0 +repage ios7L_PADx1.png
convert ${CMD} -crop 2048x1536+0+0 +repage ios7L_PADx2.png
convert ${CMD} -crop 640x960+0+0 +repage ios7P_PHx2.png
convert ${CMD} -crop 640x1136+0+0 +repage ios7L_PHx2Ret.png
convert ${CMD} -crop 320x480+0+0 +repage ios5P_PHx1.png
convert ${CMD} -crop 640x960+0+0 +repage ios5P_PHx2.png
convert ${CMD} -crop 640x1136+0+0 +repage ios5P_PHRet4.png
convert ${CMD} -crop 768x1024+0+0 +repage ios5P_PADx1NoBar.png
convert ${CMD} -crop 1536x2048+0+0 +repage ios5P_PADx2NoBar.png
convert ${CMD} -crop 768x1004+0+0 +repage ios5P_PADx1.png
convert ${CMD} -crop 1536x2008+0+0 +repage ios5P_PADx2.png
convert ${CMD} -crop 1024x768+0+0 +repage ios5L_PADx1.png
convert ${CMD} -crop 2048x1536+0+0 +repage ios5L_PADx2.png
convert ${CMD} -crop 1024x748+0+0 +repage ios5L_PADx1NoBar.png
convert ${CMD} -crop 2048x1496+0+0 +repage ios5L_PADx2NoBar.png
echo Done.

cd  ..
exit
