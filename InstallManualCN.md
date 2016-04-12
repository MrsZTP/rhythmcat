# 安装手册 #

最新版本的源代码需通过Git获取:
```
git clone http://code.google.com/p/rhythmcat/ 
```

或者从Github站点上获取:
```
git clone git://github.com/supercatexpert/RhythmCat.git
```

我们提供了源代码和在Linux下运行的二进制文件(i386版和AMD64版)，你可以在Downloads找到
它们。二进制文件可以在大多数桌面Linux发行版上运行。如果您不能运行这个程序，你需要检查程序
所需要的库是否已经被安装。如果您下载了源代码，您需要通过编译才能够运行这个程序。在编译之前，
您需要安装编译环境。在Debian/Ubuntu下，您可以运行下面的命令来安装编译所需要的包:

```
sudo apt-get install build-essential libgtk2.0-dev libgstreamer0.10-dev \
    libgstreamer-plugins-base0.10-dev libdbus-glib-1-dev
```

如果您需要播放MP3等版权受限的格式，请安装Gstreamer Bad 和 Ugly 插件。在Debian/Ubuntu下，
您可以运行以下命令来安装这些包:

```
sudo apt-get install gstreamer0.10-plugins-bad gstreamer0.10-plugins-ugly
```

然后您可以用如下的方式进行编译和安装:

```
./configure
make
sudo make install
```

如果您想要编译插件，您需要安装插件所依赖的开发库，并且进入插件所在目录。使用命令make编译它。您
必须将编译后插件所在的目录复制到~/.RhythmCat/Plugins下面。

如果您的系统中安装了GTK+ 3.0，并且想要尝试在本播放器上使用它。您可以使用以下命令进行编译:

```
./configure --enable-gtk3
make
sudo make install
```

并且使用`make -f Makefile3`命令编译插件。