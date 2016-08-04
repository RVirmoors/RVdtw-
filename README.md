# RVdtw~
An online-DTW based performance alignment and tracking external for Max/MSP.
See the included Max help patches for usage instructions. The system architecture is described in this paper:

* "An Online Tempo Tracker for Automatic Accompaniment based on Audio-to-audio Alignment and Beat Tracking", G. Burloiu. In Sound and Music Computing (SMC), 2016.

For specific details on the tempo models' implementation, see:

* "Online Score-agnostic Tempo Models for Automatic Accompaniment", G. Burloiu. In Machine Learning and Music (MML16), 2016.

## Latest release

Download the latest release compiled for Windows and OSX [here](https://github.com/RVirmoors/RVdtw-/releases).

### Just looking for the online-DTW library?

Check out the [oDTW](https://github.com/RVirmoors/RVdtw-/tree/master/oDTW) subfolder.

## Compilation

The included VS2010 and Xcode projects should do the trick. Email me if you encounter problems.
I do intend to upgrade to the latest VS at some point.

### Dependencies

* MaxSDK-6.1.4 or later
* Chroma : https://github.com/adamstark/Chord-Detector-and-Chromagram
* BTrack : https://github.com/adamstark/BTrack
* FFTW
* libsamplerate (for BTrack)

### Compiling under OSX

First you need to install the FFTW & libsamplerate libraries, in the universal (FAT) versions. For FFTW, the easiest way is with Homebrew:
```
brew install fftw --universal
```
For libsoundfile, download the source ( http://www.mega-nerd.com/SRC/download.html ) and follow the instructions. At the "make" step, use:
```
make CXXFLAGS="-arch i386 -arch x86_64" CFLAGS="-arch i386 -arch x86_64" LDFLAGS="-arch i386 -arch x86_64"
```
If you get an error related to "Carbon.h", one way to get rid of it is to simply comment out the #include in that specific .c file

Once you've got these 2 libraries installed, you can go ahead and compile the Xcode project for both 32b and 64b targets.

## License

This code is made available under the GNU General Public License, version 3. Please see the included LICENSE.txt for more details.

## Acknowledgments

Many thanks to Adam Stark, whose Chroma and BTrack libraries made this program more solid and interesting :)