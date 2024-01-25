#!/bin/bash
# IP-адрес и имя пользователя удаленного сервера
remote_host="root@node.gnss.cloud"
remote_port=10975
remote_dir="/xport_v3/"

cat ~/.ssh/id_rsa.pub | ssh "$remote_host" -p "$remote_port" "mkdir -p ~/.ssh && touch ~/.ssh/authorized_keys && chmod -R go= ~/.ssh && cat >> ~/.ssh/authorized_keys"

scp -P "$remote_port" -r etc "$remote_host":/
make clean
SOURCE_DIR=`basename "$PWD"`
echo "Dir $SOURCE_DIR will be packed"
cd ..

complect="Sony" #Aquamaper, P24 etc.

tar -czf x-port.tar.gz FullProject
tar --exclude=$SOURCE_DIR/x-port_PSDKv3/psdk_lib_v3 -czf x-port.tar.gz $SOURCE_DIR
#ssh "$remote_host" -p "$remote_port" 'mkdir -P '"${remote_dir}"'x-port_PSDKv3/'
scp -P "$remote_port" x-port.tar.gz "$remote_host":"$remote_dir"

ssh "$remote_host" -p "$remote_port" '
systemctl stop x-port;
cd '"${remote_dir}"'
mkdir -p ./PSDKv3/;
cd PSDKv3/;
tar -xzf ../x-port.tar.gz;
cd FullProject/
rm -r cmake-build-debug/;
rm -r CMakeFiles/;
cd x-port_PSDKv3/;
rm -r cmake-build-debug/;
rm -r CMakeFiles/;
rm apptest;
rm CMakeCache.txt;
cd ../;
cmake -DCOMPLECT='"${complect}"' CMakeLists.txt;
make clean;
make;
cd x-port_PSDKv3/
cp apptest /xport/apptest-camera/;
cp -r Complects/'"${complect}"'/widget /xport/apptest-camera/;
cp start.sh /xport/apptest-camera/;
systemctl daemon-reload;
echo \"Apptest ready\" | wall;
systemctl start x-port;
'
echo $(date)
