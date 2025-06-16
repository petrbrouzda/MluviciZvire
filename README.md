# MluvícíZvíře

Rekvizita na tábor: Mluvící zvíře. Nebo třeba divotvorný klobouk.

Základní kouzlo je v ovládání pomocí dálkového ovladače. Malinký ovladač se čtyřmi tlačítky má někdo v kapse a nenápadně tím dává zařízení pokyn, co má přehrávat. Na rozdíl od ovládání podobných věcí přes mobil je to zcela nenápadné. 

Na SD kartě jsou čtyři adresáře a v každém je víc souborů - po každém stisku tlačítka zařízení náhodně vybere jeden z nich a přehraje ho. Takže když si třeba jeden adresář a jedno tlačítko vyhradíte na různé varianty "ano" (jasně, určitě, správně, ...) a druhé na "ne" (nikdy, ani náhodou, to ses podělal, ...), může třeba mluvící papoušek smysluplně odpovídat na dotazy typu ano/ne a navádět děti k nalezení pokladu - když položí správné otázky.

## Detailnější popis funkce

Zvuky jsou na SD kartě; lze je měnit, když jí vyndáte a připojíte k telefonu (= je třeba si na tábor vzít čtečku SD karet, co se dá připojit k telefonu.)

Zvuky jsou ve složkách 01, 02 … 99, jednotlivé soubory se jmenují 001.mp3, 002.mp3 atd. Musí se jmenovat takto číselně, přehrávač to jinak neumí. 
Soubory **musí být MP3;** novější M4U, které dělají novější telefony, přehrávač neumí. Pokud váš telefon dělá něco jiného než MP3, stáhněte si alternativní aplikaci [Hlasový záznamník](https://play.google.com/store/apps/details?id=com.media.bestrecorder.audiorecorder).

Dálkovým ovladačem se přehrávají MP3 ze složek 01 (tlačítko A), 02 (B), 03 (C) a 04 (D).
Předpokládá se, že ve složce je 5 MP3 souborů a přehraje to náhodně jeden z nich - aby se po každém stisku tlačítka pokud možno ozvalo něco jiného.
Nastavení, která složka je pro které tlačítko, a kolik souborů v ní je, se dá dělat přes web.
				
Přes web se taky dá nastavit hlasitost a přehrát nějaký konkrétní soubor. Přes web je vidět **stav akumulátoru.** Akumulátor se nabíjí přes USB-C.

Při startu zařízení se přehraje jedno uvítání ze složky 98 (náhodně jeden z 5 souborů)
a pokud je slabý akumulátor, přehraje se soubor číslo 1 ze složky 99.


Ovladač: 
![](/doc/ovladac.jpg) 

Zařízení:
![](/doc/montaz2.jpg) 

Konfiguraci kontrolu stavu zařízení a samozřejmě též přehrávání jednotlivých MP3 lze dělat přes webový prohlížeč přes spuštěné WiFi AP.
* Wifi AP: Talk*to*me*please
* heslo: NeukecasMe

Webový server pak běží na http://192.168.1.1/ , ale WiFi by si mělo samo otevřít prohlížeč po připojení - jako captive portál ("Přihlašte se do sítě WiFi").
Konfigurace wifi a IP adres je v souboru [EasyWebServer_config.h](/KouzelnyTelefon/EasyWebServer_config.h).

Aby spolehlivě fungovalo připojení na webserver, doporučuju:
* na mobilce vypnout data,
* na telefonech Xiaomi následně potvrdit, že má zůstat připojený na WiFi, i když v ní není připojení k internetu.

Webová aplikace:

![](/doc/web-prehravani.png) 

Webová aplikace - nastavení funkce pro dálkové ovládání:

![](/doc/web-nastaveni.png) 

Pokud dáte pokyn k přehrání MP3, co na kartě není, po dalším "Obnov stav" se vypíše ve webu chyba:

![](/doc/web-chyba.png) 


## SD karta v přehrávači

Doporučuji SD kartu naformátovat na FAT32.

Na SD kartě musí být adresáře...
A pak tam jsou potřeba nějaké další adresáře pojmenované 01-99 se soubory 001.mp3-999.mp3, které se budou přehrávat podle nastavení - mapování se nastaví přes webserver.

Pokud karta nefunguje, je třeba zkontrolovat, že je formátovaná v MBR rezimu (s GPT partition table to nefunguje) - zmíněno zde 
https://forum.digikey.com/t/dfplayer-mini-communication-issue/18159/24 a popis, jak to rozlišit, je zde: https://www.tenforums.com/tutorials/84888-check-if-disk-mbr-gpt-windows.html



## Zapojení, použité součástky a konfigurace

Verze desky ESP32 v Arduino IDE **musí** být 2.0.x (nyní 2.0.17). Na 3.0.x to fungovat nebude.

Použité moduly:
* ESP32-C3 supermini https://s.click.aliexpress.com/e/_oCdelcb
* MP3 přehrávač https://www.laskakit.cz/audio-mini-mp3-prehravac/ (popisek na modulu je "MP3-TF-16P V3.0")
* Přijímač dálkového ovládání - https://www.laskakit.cz/prijimac-433mhz-ev1527-4-kanaly/ 
* Vysílač dálkového ovládání - https://www.laskakit.cz/en/dalkovy-ovladac-bezdratovy-433mhz-ev1527-4-kanaly/
* Step-up z liion baterky na 5 V - https://www.laskakit.cz/laskakit-bat-boost-menic-5v-0-6a-dio6605b/
* Ochrana a nabíječka baterky - https://www.laskakit.cz/nabijecka-li-ion-clanku-tp4056-s-ochranou-usb-c/
* Lithiová baterka 18650 nebo libovolná jiná.

Schema je v [adresáři doc](/doc/schema.svg).

Konfigurace desky:
* deska: ESP32C3 Dev module
* flash mode: DIO (jinak tahle deska neběží!)
* CDC On Boot: enabled
* CPU speed: 80 MHz
* flash speed: 40 MHz
* partition scheme: default 4 MB with SPIFFS (1.2 MB APP/1.5 MB SPIFFS)

Knihovny:
* MP3 přehrávač: https://github.com/DFRobot/DFRobotDFPlayerMini
* Web server: https://github.com/ESP32Async/ESPAsyncWebServer a  https://github.com/ESP32Async/AsyncTCP 
* ESP32 analog read: https://github.com/madhephaestus/ESP32AnalogRead
* Tasker: https://github.com/joysfera/arduino-tasker

Detailní výpis z kompilace:
```
FQBN: esp32:esp32:esp32c3:CDCOnBoot=cdc,CPUFreq=80,FlashFreq=40,FlashMode=dio 
Using library DFRobotDFPlayerMini at version 1.0.6 in folder: E:\dev.moje\arduino\libraries\DFRobotDFPlayerMini 
Using library DNSServer at version 2.0.0 in folder: C:\Users\brouzda\AppData\Local\Arduino15\packages\esp32\hardware\esp32\2.0.17\libraries\DNSServer 
Using library WiFi at version 2.0.0 in folder: C:\Users\brouzda\AppData\Local\Arduino15\packages\esp32\hardware\esp32\2.0.17\libraries\WiFi 
Using library Async TCP at version 3.4.0 in folder: E:\dev.moje\arduino\libraries\Async_TCP 
Using library ESP Async WebServer at version 3.7.7 in folder: E:\dev.moje\arduino\libraries\ESP_Async_WebServer 
Using library FS at version 2.0.0 in folder: C:\Users\brouzda\AppData\Local\Arduino15\packages\esp32\hardware\esp32\2.0.17\libraries\FS 
Using library ESP32AnalogRead at version 0.3.0 in folder: E:\dev.moje\arduino\libraries\ESP32AnalogRead 
Using library Tasker at version 2.0.3 in folder: E:\dev.moje\arduino\libraries\Tasker 
Using library SPIFFS at version 2.0.0 in folder: C:\Users\brouzda\AppData\Local\Arduino15\packages\esp32\hardware\esp32\2.0.17\libraries\SPIFFS 
```

