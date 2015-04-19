#assuming install under /root

#ats
wget "http://apache.spinellicreations.com/trafficserver/trafficserver-5.2.0.tar.bz2"
tar -xvf trafficserver-5.2.0.tar.bz2
sudo yum install gcc-c++ openssl-devel tcl-devel expat-devel pcre-devel
cd ./trafficserver-5.2.0
./configure --prefix=/opt/ats --enable-debug
make
make check
sudo make install
cd ..

#free diameter
wget "http://www.freediameter.net/hg/freeDiameter/archive/1.2.0.tar.gz"
tar -xvf 
sudo yum install cmake bison libgcrypt-devel gnutls gnutls-dev
mkdir fDbuild
cd fDbuild
cmake -DCMAKE_INSTALL_PREFIX:STRING="/opt/fd" -DCMAKE_BUILD_TYPE:STRING="Debug" -DDISABLE_SCTP:BOOL=ON -DBUILD_TEST_APP:BOOL=ON /root/freeDiameter-1.2.0
make
make install
cd ..

#traffic report
