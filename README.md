# RVdtw-
An online-DTW based automatic alignment external for Max/MSP. Full documentation coming soon!

#################
### Libraries used
#################

BTrack
Chroma
fftw
libsamplerate

# Compiling under OSX

First you need to install the FFTW & libsamplerate libraries, in the universal (FAT) versions. For FFTW, the easiest way is with Homebrew:

brew install fftw --universal

For libsoundfile, download the source ( http://www.mega-nerd.com/SRC/download.html ) and follow the instructions. At the "make" step, use:

make CXXFLAGS="-arch i386 -arch x86_64" CFLAGS="-arch i386 -arch x86_64" LDFLAGS="-arch i386 -arch x86_64"

If you get an error related to "Carbon.h", one way to get rid of it is to simply comment out the #include in that specific .c file

Once you've got these 2 libraries install, you can go ahead and compile the XCode project for both 32b and 64b targets.