
sudo apt-get install libxext-dev
sudo apt-get install libxdamage-dev
sudo apt-get install libx11-xcb-dev
sudo apt-get install libxcb-glx0-dev
sudo apt-get install libxcb-dri2-0-dev
sudo apt-get install libxcb-dri3-dev
sudo apt-get install libxcb-present-dev
sudo apt-get install libxshmfence-dev
sudo apt-get install libudev-dev
sudo apt-get install libexpat1-dev
sudo apt-get install LLVM
sudo apt-get install gettext


wget http://archive.ubuntu.com/ubuntu/pool/main/x/x11proto-gl/x11proto-gl_1.4.16.orig.tar.gz
tar zxf x11proto-gl_1.4.16.orig.tar.gz
cd glproto-1.4.16
./configure
sudo make install
cd ..

wget http://mirror.gnomus.de/extra/os/i686/dri2proto-2.8-2-any.pkg.tar.xz
http://cgit.freedesktop.org/xorg/proto/dri2proto/tag/?id=dri2proto-2.8
./autoconfig
sudo make install
cd ..

wget http://xorg.freedesktop.org/archive/individual/proto/dri3proto-1.0.tar.gz
tar zxf dri3proto-1.0.tar.gz
cd dri3proto-1.0
./configure
sudo make install
cd ..

wget http://xorg.freedesktop.org/archive/individual/proto/presentproto-1.0.tar.gz
tar zxf presentproto-1.0.tar.gz
cd presentproto-1.0
./configure
sudo make install
cd ..

wget https://ftp.gnu.org/gnu/libtool/libtool-2.4.2.tar.gz
tar zxf libtool-2.4.2.tar.gz
cd libtool-2.4.2
./configure
sudo make install




./configure --enable-osmesa --with-osmesa-bits=32