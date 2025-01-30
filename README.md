# esp-idf-uart2mqtt
UART to MQTT bridge for ESP-IDF.   

![Image](https://github.com/user-attachments/assets/200f57e3-0f80-404c-a970-8004dac74c69)

# Software requirements
ESP-IDF V5.0 or later.   
ESP-IDF V4.4 release branch reached EOL in July 2024.   


# Installation

```
git clone https://github.com/nopnop2002/esp-idf-uart2mqtt
cd esp-idf-uart2mqtt/
idf.py menuconfig
idf.py flash
```

# Configuration
![Image](https://github.com/user-attachments/assets/79b55614-73c7-4c90-9836-0d180dac868e)
![Image](https://github.com/user-attachments/assets/083b1ea2-7fd6-4269-a2c3-288210f1fe29)

## WiFi Setting
Set the information of your access point.   
![Image](https://github.com/user-attachments/assets/ff07ce55-c33f-4f25-9320-4b69b9e349ee)

## UART Setting
Set the information of UART Connection.   
![Image](https://github.com/user-attachments/assets/480dbb4e-5cda-4281-b46c-3b80675bd362)

## Broker Setting
Set the information of your MQTT broker.   
![Image](https://github.com/user-attachments/assets/ad103632-86dc-4eb0-958e-b633c6107016)

### Select Transport   
This project supports TCP,SSL/TLS,WebSocket and WebSocket Secure Port.   

- Using TCP Port.   
 TCP Port uses the MQTT protocol.   

- Using SSL/TLS Port.   
 SSL/TLS Port uses the MQTTS protocol instead of the MQTT protocol.   

- Using WebSocket Port.   
 WebSocket Port uses the WS protocol instead of the MQTT protocol.   

- Using WebSocket Secure Port.   
 WebSocket Secure Port uses the WSS protocol instead of the MQTT protocol.   

__Note for using secure port.__   
The default MQTT server is ```broker.emqx.io```.   
If you use a different server, you will need to modify ```getpem.sh``` to run.   
```
chmod 777 getpem.sh
./getpem.sh
```

WebSocket/WebSocket Secure Port may differ depending on the broker used.   
If you use a different MQTT server than the default, you will need to change the port number from the default.   

### Specifying an MQTT Broker   
You can specify your MQTT broker in one of the following ways:   
- IP address   
 ```192.168.10.20```   
- mDNS host name   
 ```mqtt-broker.local```   
- Fully Qualified Domain Name   
 ```broker.emqx.io```

### Select MQTT Protocol   
This project supports MQTT Protocol V3.1.1/V5.   
![Image](https://github.com/user-attachments/assets/0f15eb1b-9ab3-4eed-b7f0-edfafbe9be53)

### Enable Secure Option
Specifies the username and password if the server requires a password when connecting.   
[Here's](https://www.digitalocean.com/community/tutorials/how-to-install-and-secure-the-mosquitto-mqtt-messaging-broker-on-debian-10) how to install and secure the Mosquitto MQTT messaging broker on Debian 10.   
![Image](https://github.com/user-attachments/assets/de867fe9-018c-46ca-ba3c-664eabe36bb2)

# How to use   

## Write this sketch on Arduino Uno.   
You can use any AtMega microcontroller.   

```
unsigned long lastMillis = 0;

void setup() {
  Serial.begin(115200);
}

void loop() {
  while (Serial.available()) {
    String command = Serial.readStringUntil('\n');
    Serial.println(command);
  }

  if(lastMillis + 1000 <= millis()){
    Serial.print("Hello World ");
    Serial.println(millis());
    lastMillis += 1000;
  }

  delay(1);
}
```

## Connect ESP32 and AtMega328 using wire cable   

|AtMega328||ESP32|ESP32S3|ESP32C2/C3/C6|
|:-:|:-:|:-:|:-:|:-:|
|TX|--|GPIO16|GPIO2|GPIO1|
|RX|--|GPIO17|GPIO1|GPIO0|
|GND|--|GND|GND|GND|

__You can change it to any pin using menuconfig.__   


# How to use
A MQTT-Client app is required.   
There are many applications available on the Internet.   
I used [this](https://mqtt-explorer.com/) application.   
This application works not only on Windows, but also on Linux and Mac.   
There are many other applications available.   

## Subsclibe from ESP32
![Image](https://github.com/user-attachments/assets/8d10dc20-4269-4674-893f-d9801c961b05)

## Publish to ESP32
![Image](https://github.com/user-attachments/assets/ab399efb-cf58-4272-9929-2a75f743979b)


# References

https://github.com/nopnop2002/esp-idf-web-serial

https://github.com/nopnop2002/esp-idf-uart2bt

https://github.com/nopnop2002/esp-idf-uart2udp



