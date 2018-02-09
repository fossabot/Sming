# Sming
Sming - Open Source framework for high efficiency WiFi SoC ESP8266 native development with C++ language.

[![ESP8266 C++ development framework](https://github.com/SmingHub/Sming/wiki/images/small/combine.png)](https://github.com/SmingHub/Sming/wiki/examples)
[![FOSSA Status](https://app.fossa.io/api/projects/git%2Bgithub.com%2Fslaff%2FSming.svg?type=shield)](https://app.fossa.io/projects/git%2Bgithub.com%2Fslaff%2FSming?ref=badge_shield)

[![Gitter (chat)](https://badges.gitter.im/Join%20Chat.svg)](https://gitter.im/SmingHub/Sming?utm_source=badge&utm_medium=badge&utm_campaign=pr-badge)
[![Donate](http://img.shields.io/paypal/donate.png?color=yellow)](https://www.paypal.com/cgi-bin/webscr?cmd=_donations&business=WAQ8XDHCKU3PL&lc=US&item_name=Sming%20Framework%20development&item_number=sming&currency_code=USD&bn=PP%2dDonationsBF%3abtn_donateCC_LG%2egif%3aNonHosted)
[![Download](https://img.shields.io/badge/download-~1.7M-orange.svg)](https://github.com/SmingHub/Sming/releases/latest)
[![Build](https://travis-ci.org/slaff/Sming.svg?branch=tasty)](https://travis-ci.org/slaff/Sming)

## "Tasty" branch
This branch contains all "tasty" features that are not merged into the official Sming develop branch.
For the moment these include:
- Be able to configure the address location of rBoot roms. Be able to use the factory-default reset rom.
- Allow setting the wifi station configuration without persisting it in flash. ( [PR 734](https://github.com/SmingHub/Sming/pull/734) )
- Better Heap Allocation. ( [PR 696](https://github.com/SmingHub/Sming/pull/696) )
- SSL with the help of [AXTLS-8266](https://github.com/igrr/axtls-8266) ( [PR 596](https://github.com/SmingHub/Sming/pull/596))
- MQTT callback on delivering a message with QoS 1 or 2. ( [PR 617](https://github.com/SmingHub/Sming/pull/617) )
- ~~Fix for loosing final bytes when using rBoot Over-The-Air. ( [PR 769](https://github.com/SmingHub/Sming/pull/769) )~~
- ~~Faster interrupts so that there is less flickering. ( [PR 774](https://github.com/SmingHub/Sming/pull/774) )~~
- ~~MQTT: Better memory usage and no limits for username, password and clientid.~~ Merged 
- ~~Fix for slow booting.~~ Merged
- ~~rBoot Makefile switch to enable the switch temp functions.~~ Merged
- ~~Debug support with the help of [ESPGDBStub](https://github.com/espressif/esp-gdbstub)~~ Merged
- ~~rBoot with support for temporary switching to a ROM.~~ Merged

## Summary
* Fast & user friendly development
* Work with GPIO in Arduino style
* High effective in perfomance and memory usage (this is native firmware!)
* Compatible with standard Arduino libraries - use any popular hardware in few lines of code
* rBoot OTA firmware updating
* Built-in file system: [spiffs](https://github.com/pellepl/spiffs)
* Built-in powerfull network and wireless modules
* Built-in JSON library: [ArduinoJson](https://github.com/bblanchon/ArduinoJson)
* HTTP, AJAX, WebSockets support
* MQTT protocol based on [libemqtt] (https://github.com/menudoproblema/libemqtt)
* Networking based on LWIP stack
* Simple and powerfull hardware API wrappers
* Based on Espressif NONOS SDK. Tested with versions 1.4 and 1.5. 

## Latest Release
- [Sming V2.1.0](https://github.com/SmingHub/Sming/releases/tag/2.1.0)

## Getting started
- [Windows](https://github.com/SmingHub/Sming/wiki/Windows-Quickstart)
- [Linux](https://github.com/SmingHub/Sming/wiki/Linux-Quickstart)
- [MacOS](https://github.com/SmingHub/Sming/wiki/MacOS-Quickstart)

## Additional needed software 
- Spiffy  : Source included in Sming repository
- [ESPtool2] (https://github.com/raburton/esptool2) esptool2 

You can find more information about compilation and flashing process by reading esp8266.com forum discussion thread.

## Examples
More information at **[Wiki Examples](https://github.com/SmingHub/Sming/wiki/examples)** page.

### Simple GPIO input/output
```c++
#define LED_PIN 2 // GPIO2
...
pinMode(LED_PIN, OUTPUT);
digitalWrite(LED_PIN, HIGH);
```

### Connect to WiFi and start Serial communication
```c++
Serial.begin(9600);
Serial.println("Hello Sming! Let's do smart things.");

WifiStation.enable(true);
WifiStation.config("LOCAL-NETWORK", "123456789087"); // Put you SSID and Password here
```

### Read DHT22 sensor
```c++
#include <Libraries/DHT/DHT.h> // This is just popular Arduino library!

#define WORK_PIN 0 // GPIO0
DHT dht(WORK_PIN, DHT22);

void init()
{
  dht.begin();

  float h = dht.readHumidity();
  float t = dht.readTemperature();
}
```

### HTTP client
```c++
HttpClient thingSpeak;
...
thingSpeak.downloadString("http://api.thingspeak.com/update?key=XXXXXXX&field1=" + String(sensorValue), onDataSent);

void onDataSent(HttpClient& client, bool successful)
{
  if (successful)
    Serial.println("Successful!");
  else
    Serial.println("Failed");
}
```

### OTA application update based on rBoot
```c++
void OtaUpdate() {
	
	uint8 slot;
	rboot_config bootconf;
	
	Serial.println("Updating...");
	
	// need a clean object, otherwise if run before and failed will not run again
	if (otaUpdater) delete otaUpdater;
	otaUpdater = new rBootHttpUpdate();
	
	// select rom slot to flash
	bootconf = rboot_get_config();
	slot = bootconf.current_rom;
	if (slot == 0) slot = 1; else slot = 0;

	// flash rom to position indicated in the rBoot config rom table
	otaUpdater->addItem(bootconf.roms[slot], ROM_0_URL);

	// and/or set a callback (called on failure or success without switching requested)
	otaUpdater->setCallback(OtaUpdate_CallBack);

	// start update
	otaUpdater->start();
}
```

### Embedded HTTP WebServer
```c++
server.listen(80);
server.addPath("/", onIndex);
server.addPath("/hello", onHello);
server.setDefaultHandler(onFile);

Serial.println("=== WEB SERVER STARTED ===");
Serial.println(WifiStation.getIP());

...

void onIndex(HttpRequest &request, HttpResponse &response)
{
  TemplateFileStream *tmpl = new TemplateFileStream("index.html");
  auto &vars = tmpl->variables();
  vars["counter"] = String(counter);
  vars["IP"] = WifiStation.getIP().toString();
  vars["MAC"] = WifiStation.getMAC();
  response.sendTemplate(tmpl);
}

void onFile(HttpRequest &request, HttpResponse &response)
{
  String file = request.getPath();
  if (file[0] == '/')
    file = file.substring(1);
    
  response.setCache(86400, true);
  response.sendFile(file);
}
```


## License
[![FOSSA Status](https://app.fossa.io/api/projects/git%2Bgithub.com%2Fslaff%2FSming.svg?type=large)](https://app.fossa.io/projects/git%2Bgithub.com%2Fslaff%2FSming?ref=badge_large)