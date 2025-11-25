Docker:

docker build -t dockerusername/esp8266-panel-react .
docker run --rm -p 8080:8080 dockerusername/esp8266-panel-react
http://localhost:8080


vscode:
--- Terminal on COM3 | 115200 8-N-1
--- Available filters and text transformations: colorize, debug, default, direct, esp8266_exception_decoder, hexlify, log2file, nocontrol, printable, send_on_enter, time
--- More details at https://bit.ly/pio-monitor-filters
--- Quit: Ctrl+C | Menu: Ctrl+T | Help: Ctrl+T followed by Ctrl+H
...
Connected! IP: 192.168.0.105
SoftAP up: SSID=esp8266-setup  PASS=12345678  URL: http://192.168.4.1/
HTTP server started
NTP syncing...
mDNS: http://esp8266.local/



