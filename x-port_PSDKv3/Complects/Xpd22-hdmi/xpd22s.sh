#!/bin/bash
COMMAND_FILE="/xport/camdata/xpd22cmd"


echo 13 > /sys/class/gpio/export
echo 14 > /sys/class/gpio/export
echo 15 > /sys/class/gpio/export

echo out > /sys/class/gpio/gpio13/direction
echo out > /sys/class/gpio/gpio14/direction
echo out > /sys/class/gpio/gpio15/direction


set1()
{
 echo 1 > /sys/class/gpio/gpio15/value
 echo 1 > /sys/class/gpio/gpio14/value
 echo 0 > /sys/class/gpio/gpio14/value
}

set0()
{
 echo 0 > /sys/class/gpio/gpio15/value
 echo 1 > /sys/class/gpio/gpio14/value
 echo 0 > /sys/class/gpio/gpio14/value
}

setflush()
{
 echo 1 > /sys/class/gpio/gpio13/value
 echo 0 > /sys/class/gpio/gpio13/value
}


usbpwr="off"
pwr="off"
af="off"
shutter="off"

clear()
{
 echo "command: "
 set0
 set0
 set0
 set0

 if [ $usbpwr == "on" ] 
 then 
  echo "usb 1"
  set1
 else
  echo "usb 0"
  set0
 fi

 if [ $pwr == "on" ] 
 then 
  echo "pwr 0 (on)"
  set0
 else
  echo "pwr 1 (off)"
  set1
 fi

 if [ $af == "on" ] 
 then 
  echo "af 1"
  set1
 else
  echo "af 0"
  set0
 fi

 if [ $shut == "on" ] 
 then 
  echo "shut 1"
  set1
 else
  echo "shut 0"
  set0
 fi

 set0
 set0
 set0
 set0
 set0
 set0
 set0
 set0
 setflush
}


term() 
{
 echo "turn off"
 pwr="off"
 usbpwr="off"
 af="off"
 shut="off"
 clear
 exit 0
}

trap term 2 9 15

pwr="on"
usbpwr="on"
shut="on"
af="on"
clear

while true; do
  if [ -f "$COMMAND_FILE" ]; then
    command=$(cat $COMMAND_FILE)
    echo "command $command"

    if [ "$command" = "pwron" ]; then
      pwr="on"
      clear
    elif [ "$command" = "pwroff" ]; then
      pwr="off"
      clear
    elif [ "$command" = "usbon" ]; then
      usbpwr="on"
      clear
    elif [ "$command" = "usboff" ]; then
      usbpwr="off"
      clear
    elif [ "$command" = "afon" ]; then
      af="on"
      clear
    elif [ "$command" = "afoff" ]; then
      af="off"
      clear
    elif [ "$command" = "shuton" ]; then
      shut="on"
      clear
    elif [ "$command" = "shutoff" ]; then
      shut="off"
      clear
    fi
    rm -f $COMMAND_FILE
  fi
  sleep 0.01
done