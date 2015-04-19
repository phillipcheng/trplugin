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

#libconfuse
wget http://savannah.nongnu.org/download/confuse/confuse-2.7.tar.gz
tar -xvf confuse-2.7.tar.gz
cd confuse-2.7
./configure
./make
./make install
cd ..

#traffic report
git https://github.com/phillipcheng/trplugin
cd trplugin/src
./make

#configure free diameter client
DIAMETER_ID=ali1
mkdir /opt/fd/conf
cd /opt/fd/conf
openssl req -new -batch -x509 -days 3650 -nodes -newkey rsa:1024 -out ali1.cert.pem -keyout al1.privkey.pem -subj /CN=ali1
openssl dhparam -out ali1.dh.pem 1024
vi /opt/tr/conf/fd1.conf
#in Peer Identity section, set the Identity to $DIAMETER_ID
#in TLS configuration section, point the TLS_Cred, TLS_CA, TLS_DH_File to the files openssl just generated
#in Peers configuration, add the server peer: ConnectPeer="id2.mymac" { No_TLS; Port=3868, ConnectTo="52.1.96.115" };
