#!/bin/bash
# IP-адрес и имя пользователя удаленного сервера
remote_host="nikita@localhost"
remote_port=22
remote_dir="/home/nikita/Projects/Topodrone/Dronya/"

cat ~/.ssh/id_rsa.pub | ssh "$remote_host" -p "$remote_port" "mkdir -p ~/.ssh && touch ~/.ssh/authorized_keys && chmod -R go= ~/.ssh && cat >> ~/.ssh/authorized_keys"

scp -P "$remote_port" -r etc "$remote_host":/
make clean
SOURCE_DIR=`basename "$PWD"`
echo "Dir $SOURCE_DIR will be packed"
cd ..
tar -czf x-port.tar.gz x-port_PSDKv3
tar --exclude=$SOURCE_DIR/psdk_lib_v3 -czf x-port.tar.gz $SOURCE_DIR
ssh "$remote_host" -p "$remote_port" 'mkdir -P '"${remote_dir}"'x-port_PSDKv3/'
scp -P "$remote_port" x-port.tar.gz "$remote_host":"$remote_dir"

ssh "$remote_host" -p "$remote_port" '
#systemctl stop x-port;
cd '"${remote_dir}"'
mkdir -p ./PSDKv3/;
cd PSDKv3/;
tar -xzf ../x-port.tar.gz;
cd x-port_PSDKv3/;
rm -r cmake-build-debug/;
rm -r CMakeFiles/;
rm apptest;
rm CMakeCache.txt;
cmake CMakeLists.txt;
make clean;
make;
cp apptest '"${remote_dir}"'xport/apptest-camera/;
cp -r widget '"${remote_dir}"'xport/apptest-camera/;
cp start.sh '"${remote_dir}"'xport/apptest-camera/;
#systemctl daemon-reload;
echo \"Apptest ready\" | wall;
#systemctl start x-port;
'
echo $(date)