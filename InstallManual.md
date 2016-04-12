# Installation Manual #

The source codes of latest version can be downloaded by Git:
```
git clone https://code.google.com/p/rhythmcat/
```

Or get the latest version from Github:
```
git clone git://github.com/supercatexpert/RhythmCat.git
```

We now provide source code and binaries for Linux on both i386 and AMD64 (you can find the package of source code and binaries in the Downloads page).
Binaries can be executed directly in most of desktop distributions of Linux. If you cannot run the program, you should check if the needed libraries are installed. Notice that if you downloaded the source code, you should compile it, then you can use this program. Before you compile it, you should check the developing environment, and the libraries which this program depends. In Debian/Ubuntu, you can run the commands below to help you install the packages that the compiling needed:

```
sudo apt-get install build-essential libgtk2.0-dev libgstreamer0.10-dev \
    libgstreamer-plugins-base0.10-dev libdbus-glib-1-dev
```

If you want to play MP3 or other Restricted Format, please install Gstreamer
Bad & Ugly plugins. In Debian/Ubuntu, you can just run:

```
sudo apt-get install gstreamer0.10-plugins-bad gstreamer0.10-plugins-ugly
```

Then you can try to compile it:

```
./configure
make
sudo make install
```

If you want to compile the plugins, you should install the development
libraries which the plugin depends, and than enter the directory where the
plugin is. Compile the plugin simply by command "make". You should put all
the directories where the compiled plugins are into ~/.RhythmCat/Plugins.

If you have GTK+ 3.0, and you want to try it on this player. You can compile it
by:

```
./configure --enable-gtk3
make
sudo make install
```

And compile the plugins by command `make -f Makefile3` instead.