
Remembering how to do this...

SEE: https://developer.apple.com/library/ios/documentation/UserExperience/Conceptual/MobileHIG/IconMatrix.html#//apple_ref/doc/uid/TP40006556-CH27-SW2

Splash Screen
=============

As per instructions here: https://github.com/spren9er/s9splashgen

Create main splash screen using GIMP (resize canvas to 2142x2208 for iphone only - flatten and fill).

Then:

	./scripts/s9splashgen.rb splash.png 

Will generate the necessary images in screens/

Drag these to your project Images.xcassets resource bundle and add to appropriate LaunchImage image set...

(In the above case, only iPhone images are generated, and consequently matched up to iPhone sizes in the image set...)

Icons
=====

Again, as per instructions here: https://github.com/spren9er/s9icongen

Similarly, use GIMP to create base icon.png of 1024x1024.

Then:

	./scripts/s9icongen.rb icon.png

Will generate the necessary icons ins icons/

Again, drag into AppIcon image set - and check just for iPhone in this case...
