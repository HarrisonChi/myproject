# myproject

dtoverlay=buzzer_overlay  (need to put into overlay)
dtoverlay=googlevoicehat-soundcard
dtoverlay=ov5647

##GPIO setting:
------------------------------------------------------------------------ 


GPIO 2: recording light
GPIO 3: recording off light
GPIO 4: buzzer

GPIO 8 ( SPICE0 ) : mcp3008 10
GPIO 9 ( SPIMISO) : mcp3008 12
GPIO 10 ( SPIMOSI ): mcp3008 11
GPIO 11 ( SPISCLK ) : mcp3008 13

GPIO 14: SG90 1   (up & down)
GPIO 15: SG90 2   (left & right)

GPIO 18: i2s SCK
GPIO 19: i2s WS
GPIO 20: i2s SD
GPIO 22-27:L298N

##software package
------------------------------------------------------------------------ 
pi5:
apt install python3-venv                    (安裝虛擬環境套件)
apt install nodejs                               (安裝nodejs)
apt install libjpeg-dev                        (編譯jpeg .c 套件)

python:
python3 -m venv “name”                   (建立虛擬環境)
pip3 install vosk                                 (語音辨識套件)

nodejs:
npm init -y                                          (初始化nodejs)
npm install express                            (express 框架套件)
npm install socket.io                          (socket 套件)
npm install  mqtt                                 (MQTT 套件)

MQTT安裝:
apt install mosquitto-clients(客戶端測試工具)
apt install mosquitto(安裝MQTT伺服器)
systemctl enable mosquitto(開機自動啟動)
systemctl start mosquitto(立即啟動)
systemctl status mosquitto(顯示狀態，確認看到active)

建立listen讓外部可連結此IP:
在 /etc/mosquitto/mosquitto.conf最下方新增
	listener 1883
	allow_anonymous true
重新啟動
systemctl restart mosquitto
