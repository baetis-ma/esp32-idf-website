# Simple Web Page
##### This project sets up a simple tcp socket with enough functionality to serve up a stand-alone .html file (with some java script to handle parsing tcp headers) and handle bi-directional data flow between the esp device and the browser. This repository was tested on a ESP32-S and on a ESP8266 D1-mini with  their respective tools packages and esp-idf frameworks. 
##### The tcp server in the esp takes the incoming IP packets combines, windows, verifies/requests retransmission, and notifies us that an http request is ready.  When I filled 192.168.0.120/index.html into my browser address bar window the contents of the tcp server (running over in my esp running a wifi connection at IP adrress 192.168.0.120) receive buffer were:
```
GET /index.html HTTP/1.1
Host: 192.168.0.120<br>
Connection: keep-alive<br>
Cache-Control: max-age=0<br>
Upgrade-Insecure-Requests: 1<br>
User-Agent: Mozilla/5.0 (X11; Linux x86_64) AppleWebKit/537.36 
(KHTML, like Gecko) Chrome/70.0.3538.77 Safari/537.36<br>
Accept: text/html,application/xhtml+xml,application/xml;q=0.9,
image/webp,image/apng,*/*;q=0.8<br>
Accept-Encoding: gzip, deflate<br>
Accept-Language: en-US,en;q=0.9,la;q=0.8<br>
```
##### All my esp parser cares about in the previous packet is that it was a GET request of the data stored in the index.html file, which is then pointed  at in the tcp server socket send command. The browser magically knows that  data returning from this request will be executed as html (maybe because it's origin was the browser's address box).
##### Any additional files you want to download (such as images, .css or .js files) will require editting the esp32-website.c (actually tcpsetup.c) program tcp_server_task() function for the specific file and adding the file to main/component.mk file. In the code just copy the  code block responding to index.html, and replace the codition statement  and the asm tag name pointers in rom.
##### This project is not very useful in itself but can be used for a framework for more complicated projects.
## Setup machine for build
##### There are plenty of sites on the web to get you up end running withesp-idf, this is an official `https://docs.espressif.com/projects/esp-idf/en/latest/get-started/` getting started guide</a> that steered me through. A few lines down in the Setup Toolchain area you can pick options for windows, linux or mac os.  Next go to what you want to be your working directory and download repository and setup directory environment depenancies :
```
git clone https://github.com/baetis-ma/esp32-idf-website</li>
export PATH="$PATH:$HOME/esp32/xtensa-esp32-elf/bin"</li>
export IDF_PATH=~/esp32/esp-idf</li>
```
##### or for the esp8266
```
git clone https://github.com/baetis-ma/esp32-idf-website
export IDF_PATH=~/esp8266/ESP8266_RTOS_SDK
export PATH=$PATH:~/esp8266/xtensa-lx106-elf/bin
```
##### Make sure to either add your user to dialout (permanent) or chmod 666 /dev/ttyUSB0  (for example) if you run into a device permission problem (linux). Also the file main/tcpsetup.c has to be editted to reflect yor own access point credentials (#define EXAMPLE_WIFI_SSID "troutstream" and #define EXAMPLE_WIFI_PASS "password").
##### Move into the esp32-idf-website directory on your compter and type 'make menuconfig'  (make sure you have exported environment variables above into that terminal's shell). You can set the tty port and change the download speed here (Serial flaser config) to 921600 baud on the esp32 (esp8266 worked at 115200 baud max). `make flash monitor` will build, download, exectute and monitor the esp. First time through it takes a few minutes to build everything. In the monitor look for the station ip address and type that into a browser address box.  Test out adding /index.html or /getData or /getData?a=1234 to the address url (check monitor output in the terminal to see what the contents of the tcp server socket's receive buffer)  A path not identified will 404, as does the favicon.ico that the browser likes to add in as an additional http request after the browser address bar request (if you want a favicon.ico add to main and update component.mk I suppose).
