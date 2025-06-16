// FQBN: esp32:esp32:esp32c3:CDCOnBoot=cdc,CPUFreq=80,FlashFreq=40,FlashMode=dio



// zapojeni desky
#include "pinout.h"

/* AsyncLogger se pouziva pro logovani udalosti v asynchronnich aktivitach (webserver...).
Uklada zaznamy do pole v pameti a ty se pak vypisou v loop() pomoci volani dumpTo(); */
#include "src/logging/AsyncLogger.h"
AsyncLogger asyncLogger;

#include "src/logging/SerialLogger.h"
SerialLogger serialLogger( &Serial );


/** Sdileny stav aplikace - objekt drzici napr. chybu inicializace MP3, aby se dala vypsat  */
#include "AppState.h"
AppState appState;


/*
Dalkovy ovladac
*/
#include "RemoteControl.h"
RemoteControl remoteControl( &serialLogger );


/*
Obsluha MP3 prehravace pres izolacni vrstvu zjednodusujici ovladani

Low-level se pouziva knihovna:
https://github.com/DFRobot/DFRobotDFPlayerMini
*/
#include "Mp3Player.h"
Mp3Player player( &serialLogger, &appState );

HardwareSerial hwSerial_1(1);

#define SLOZKA_UVITANI 98
#define POCET_UVITANI 5
#define SLOZKA_LOW_ACCU 99
#define FILE_LOW_ACCU 1

/*
WebServer - taky pres izolacni vrstvu

Pouzivaji se tyto dve knihovny:
- https://github.com/ESP32Async/ESPAsyncWebServer
- https://github.com/ESP32Async/AsyncTCP 
*/
#include "EasyWebServer.h"
EasyWebServer webserver( &asyncLogger );


/*
Konfigurace.
Je nutny alespon kousek filesystemu SPIFFS.
*/
#include "src/toolkit/BasicConfig.h"
#include "src/toolkit/ConfigProviderSpiffs.h"
BasicConfig config;
// může dostat serialLogger, protože poběží synchronně v loopu
ConfigProviderSpiffs configProvider( &serialLogger, &config, &appState);
bool saveConfigChange=false;


/*
Pro mereni kapacity baterky - kalibrace ADC mereni.
https://github.com/madhephaestus/ESP32AnalogRead 
*/
#include <ESP32AnalogRead.h>
ESP32AnalogRead adc;

/*
Periodicke ulohy
https://github.com/joysfera/arduino-tasker
*/
#include <Tasker.h>
Tasker tasker;



#include "src/toolkit/map_double.h"



int pocetMereni = 0;

// mereni baterky
void zmerAccu() {
  double vin = adc.readVoltage();
  double vreal = vin / (DELIC_R2/(DELIC_R1+DELIC_R2)) / KALIBRACE;
  if( appState.accuVoltage < 0 ) {
    // prvni mereni = rovnou nastavime
    appState.accuVoltage = vreal;
  } 
  // plovouci exponencialni prumer
  if( pocetMereni<10 ) {
    // prvnich nekolik mereni je potreba projet rychle
    appState.accuVoltage = 0.5 * appState.accuVoltage + 0.5 * vreal;
    tasker.setTimeout( zmerAccu, 1000 );
    pocetMereni++;
  } else {
    appState.accuVoltage = 0.8 * appState.accuVoltage + 0.2 * vreal;
    tasker.setTimeout( zmerAccu, 3000 );
  }
  Serial.printf( "u=%.2f V (%.2f V)\n", appState.accuVoltage, vreal );
}


/*
 * prikaz od webserveru pro prehravani
 */
bool webPlay = false;
PlayerCommand webPlayCommand;

/*
 * prikaz od webserveru 
 */
bool zmenaHlasitosti = false;
long hlasitost = 30;

/*
 * prikaz od webserveru 
 */
bool resetMp3 = false;


//---------- setup

void setup() {
  Serial.begin(115200);
  delay(3000);
  Serial.println( "Startuju!" );

  pinMode( LED, OUTPUT );
  digitalWrite( LED, LOW );

  configProvider.openFsAndLoadConfig();

  digitalWrite( LED, LOW );
  hwSerial_1.begin(9600, SERIAL_8N1, MP3_RX, MP3_TX );
  player.begin( &hwSerial_1 );

  hlasitost = config.getLong( "hlasitost", 30 );
  player.setVolume( hlasitost );

  webserver.startApAndWebserver();

  adc.attach( ACCU );
  tasker.setTimeout( zmerAccu, 1 );

  digitalWrite( LED, HIGH );

  randomSeed( millis() );
  // zahrajeme hello
  player.playFile( SLOZKA_UVITANI, 1 + random( POCET_UVITANI ), 1 );

  webPlayCommand.folder=1;
  webPlayCommand.file=1;
  webPlayCommand.repeatTimes=1;

}


/**
 * Zde nastavte routy pro svou aplikaci.
 * Je zavolano z webserveru v dobe volani webserver.startWifiAndWebserver()
 */
void userRoutes( AsyncWebServer * server )
{
  server->on("/", HTTP_GET, onRequestRoot );
  server->on("/tlacitka", HTTP_GET, onRequestTlacitka );
  server->on("/tlacitkaA", HTTP_GET, onRequestTlacitkaAction );
  server->on("/hraj", HTTP_GET, onRequestHraj );
  server->on("/hlasitost", HTTP_GET, onRequestHlasitost );
  server->on("/reset", HTTP_GET, onRequestReset );
}




// ----------- vlastni vykonna cast aplikace (loop)




void onConfigChanged() 
{
    saveConfigChange=false;
    configProvider.saveConfig();
    
    // vypsat konfiguraci
    Serial.println( "--- zacatek konfigurace" );
    config.printTo(Serial);
    Serial.println( "--- konec konfigurace" );
}


void odbavDalkoveOvladani( RemoteControlEvent rce ) {
  char folder = '-'; 
  long defaultFolder = 0;
  switch( rce ) {
    case RC_TLACITKO_A: folder = 'A'; defaultFolder = 1; break;
    case RC_TLACITKO_B: folder = 'B'; defaultFolder = 2; break;
    case RC_TLACITKO_C: folder = 'C'; defaultFolder = 3; break;
    case RC_TLACITKO_D: folder = 'D'; defaultFolder = 4; break;
  }
  if( folder=='-' ) return;
  
  char buffer[20];
  sprintf( buffer, "folder%c", folder );
  int folderId = config.getLong( buffer, defaultFolder );
  sprintf( buffer, "files%c", folder  );
  int filesInFolder = config.getLong( buffer, 5 );

  player.playFile( folderId, 1 + random( filesInFolder ), 1 );
}



void odbavWeb() {
  player.playFile( webPlayCommand.folder, webPlayCommand.file, webPlayCommand.repeatTimes );
  webPlay = false;
}

void odbavZmenuHlasitosti() {
  player.setVolume( hlasitost );
  zmenaHlasitosti = false;
}

void resetPlayer() 
{
  player.reset();
  delay( 3000 );
  player.setVolume( hlasitost );
  resetMp3 = false;
}

void loop() {

  // vypiseme asynchronni log, pokud v nem neco je
  asyncLogger.dumpTo( &Serial );

  // aby Tasker spoustel ulohy
  tasker.loop();

  RemoteControlEvent rce = remoteControl.checkState();
  if( rce!=RC_NO_EVENT ) {
    odbavDalkoveOvladani( rce );
  }

  // odbav prikaz z webu
  if( webPlay ) {
    odbavWeb();
  }

  if( zmenaHlasitosti ) {
    odbavZmenuHlasitosti();
  }

  if( resetMp3 ) {
    resetPlayer();
  }

  // udalosti z MP3 prehravace
  player.process();

  // odbavit DNS pozadavky
  webserver.processDNS();

  // pokud uzivatel zmenil nastaveni, ulozit do souboru a promitnout, kde je potreba
  // nezavesujeme primo na config.isDirty(), protoze bychom mohli trefit nekonzistenci pri nastavovani asynchronne z webserveru
  if( saveConfigChange ) {
    onConfigChanged();
  }

}




// -------------- odsud dal je webserver

const char htmlHlavicka[] PROGMEM = R"rawliteral(
  <!DOCTYPE HTML><html>
  <head>
    <title>Mluvící zvíře</title>
    <meta name="viewport" content="width=device-width, initial-scale=1">
    <style>
      html {font-family: Arial; display: inline-block; text-align: left;}
      h2 {font-size: 1.8rem;}
      h3 {font-size: 1.45rem; font-weight: 600}
      p {font-size: 1.2rem;}
      input {font-size: 1.2rem;}
      input#text {width: 100%;}
      select {font-size: 1.2rem;}
      form {font-size: 1.2rem;}
      body {max-width: 600px; margin:10px ; padding-bottom: 25px;}
    </style>
  </head>
  <body>
)rawliteral";

const char htmlPaticka[] PROGMEM = R"rawliteral(
  </body>
  </html>
)rawliteral";




const char htmlZacatek[] PROGMEM = R"rawliteral(
  <h1>Mluvící zvíře</h1>
  <form method="GET" action="/?">
  <input type="submit" name="obnov" value="Obnov stav" > 
  </form>
)rawliteral";


void vlozInformace( AsyncResponseStream *response )  {
  if( appState.isProblem() ) {
    response->printf( "<p><b>%s:</b> [%s] před %d sec.</p>",
      appState.globalState==ERROR ? "Chyba" : "Varování",
      appState.problemDesc,
      (millis()-appState.problemTime) / 1000L
    );  
  }
  
  if( appState.accuVoltage > 0 ) {
    response->printf( "<p>Akumulátor: <b>%.0f %%</b> (%.2f V)</p>",
      map_double( appState.accuVoltage, 3.3, 4.2, 0, 100 ),
      appState.accuVoltage
    );
  }
}

void vlozUptime( AsyncResponseStream *response ) {
  response->printf( "<br><hr><br><p>Čas od spuštění zařízení: %d min</p>",
    millis()/60000L
  );
}


/** 
 * Je lepe misto Serial.print pouzivat AsyncLogger.
 * Je volano z webserveru asynchronne.
 * Nevolat odsud dlouhotrvajici akce!
 * I logovani by melo byt pres asyncLogger!
 * 
 * Kazda funkce onRequest* musi byt zaregistrovana v userRoutes()
 * 
 */
void onRequestRoot(AsyncWebServerRequest *request){
  asyncLogger.log( "@ req root" );

  AsyncResponseStream *response = request->beginResponseStream(webserver.HTML_UTF8);
  response->print( htmlHlavicka );
  response->print( htmlZacatek );
  vlozInformace( response );
  response->print( "<p><a href=\"/tlacitka\">Nastavení tlačítek dálkového ovladače</a>" );

  response->print( "<br><hr><br><b>Přehraj</b><br><form action=\"/hraj\" method=\"GET\">"  );
  response->printf( "Složka: <input type=\"number\" name=\"folder\" min=\"1\" max=\"99\" value=\"%d\"><br>", webPlayCommand.folder );
  response->printf( "Soubor: <input type=\"number\" name=\"file\" min=\"1\" max=\"99\" value=\"%d\"><br>", webPlayCommand.file );
  response->printf( "Kolikrát: <input type=\"number\" name=\"repeat\" min=\"1\" max=\"99\" value=\"%d\"><br>", webPlayCommand.repeatTimes );
  response->print( "<input type=\"submit\" value=\"Přehraj!\"></form>" );

  if( webPlayCommand.file>1 ) {
    int prevFile = webPlayCommand.file-1 ;
    response->printf( "<p><a href=\"/hraj?folder=%d&file=%d&repeat=%d\">[&lt;&lt; Předešlý (složka %d, soubor %d) &lt;&lt;]</a></p>",
      webPlayCommand.folder,
      prevFile,
      webPlayCommand.repeatTimes,
      webPlayCommand.folder,
      prevFile
    );
  }
  
  int nextFile = webPlayCommand.file+1;
  response->printf( "<p><a href=\"/hraj?folder=%d&file=%d&repeat=%d\">[&gt;&gt; Další (složka %d, soubor %d) &gt;&gt;]</a></p>",
    webPlayCommand.folder,
    nextFile,
    webPlayCommand.repeatTimes,
    webPlayCommand.folder,
    nextFile
   );

  response->print( "<br><hr><br><b>Hlasitost (1-30)</b><br><form action=\"/hlasitost\" method=\"GET\">"  );
  response->printf( "Hlasitost: <input type=\"number\" name=\"volume\" min=\"1\" max=\"30\" value=\"%d\"><br>", hlasitost );
  response->print( "<input type=\"submit\" value=\"Nastav!\"></form>" );  
  
  response->print( "<br><hr><br><b>Reset přehrávače</b><br><form action=\"/reset\" method=\"GET\">"  );
  response->print( "<input type=\"submit\" value=\"Proveď reset!\"></form>" );  

  vlozUptime( response );
  response->print( htmlPaticka );
  request->send(response);

  // variantne: request->redirect("/#0");
}


const char htmlTlacitka1[] PROGMEM = R"rawliteral(
  <h1>Dálkový ovladač</h1>
  <p><a href="/">Zpět</a></p>
  <form action="/tlacitkaA" method="GET">
)rawliteral";

void onRequestTlacitka(AsyncWebServerRequest *request){
  asyncLogger.log( "@ req tlacitka" );

  long numberLen = config.getLong( "NumberLen", 5 );
  long items = config.getLong( "items", 3 );

  AsyncResponseStream *response = request->beginResponseStream(webserver.HTML_UTF8);
  response->print( htmlHlavicka );
  response->print( htmlTlacitka1 );
  response->print( "<b>Tlačítko A</b><br>");
  response->printf( "Složka se soubory:  <input type=\"number\" name=\"folderA\" min=\"1\" max=\"99\" value=\"%d\"><br>",
      config.getLong( "folderA", 1 ) );
  response->printf( "Počet souborů:  <input type=\"number\" name=\"filesA\" min=\"1\" max=\"99\" value=\"%d\"><br>",
      config.getLong( "filesA", 5 ) );
  response->print( "<b>Tlačítko B</b><br>");
  response->printf( "Složka se soubory:  <input type=\"number\" name=\"folderB\" min=\"1\" max=\"99\" value=\"%d\"><br>",
      config.getLong( "folderB", 2 ) );
  response->printf( "Počet souborů:  <input type=\"number\" name=\"filesB\" min=\"1\" max=\"99\" value=\"%d\"><br>",
      config.getLong( "filesB", 5 ) );
  response->print( "<b>Tlačítko C</b><br>");
  response->printf( "Složka se soubory:  <input type=\"number\" name=\"folderC\" min=\"1\" max=\"99\" value=\"%d\"><br>",
      config.getLong( "folderC", 3 ) );
  response->printf( "Počet souborů:  <input type=\"number\" name=\"filesC\" min=\"1\" max=\"99\" value=\"%d\"><br>",
      config.getLong( "filesC", 5 ) );
  response->print( "<b>Tlačítko D</b><br>");
  response->printf( "Složka se soubory:  <input type=\"number\" name=\"folderD\" min=\"1\" max=\"99\" value=\"%d\"><br>",
      config.getLong( "folderD", 4 ) );
  response->printf( "Počet souborů:  <input type=\"number\" name=\"filesD\" min=\"1\" max=\"99\" value=\"%d\"><br>",
      config.getLong( "filesD", 5 ) );
  response->print( "<input type=\"submit\" value=\"Uložit!\"></form><br><hr><br>" );

  response->print( "<p><a href=\"/\">Zpět</a></p>" );

  request->send(response);

  // variantne: request->redirect("/#0");
}


long getValue( AsyncWebServerRequest *request, const char * name, long defaultVal ) {
  long val = webserver.getQueryParamAsLong( request, name, defaultVal );
  if( val==defaultVal ) {
    asyncLogger.log( "polozka %s nenastavena", name );
  }
  return val;
}

void copyValueToConfig( AsyncWebServerRequest *request, const char * name )
{
  long val = getValue( request, name, -1 );
  if( val!=-1 ) {
    config.setValue( name, val );
  }
}

void onRequestTlacitkaAction(AsyncWebServerRequest *request){
  asyncLogger.log( "@ req tlacitkaAction" );
  
  copyValueToConfig( request, "folderA" );
  copyValueToConfig( request, "folderB" );
  copyValueToConfig( request, "folderC" );
  copyValueToConfig( request, "folderD" );
  copyValueToConfig( request, "filesA" );
  copyValueToConfig( request, "filesB" );
  copyValueToConfig( request, "filesC" );
  copyValueToConfig( request, "filesD" );

  saveConfigChange=true;
  request->redirect( "/" );
}

void onRequestHraj(AsyncWebServerRequest *request){
  asyncLogger.log( "@ req hraj" );
  
  long folder = getValue( request, "folder", -1 );
  long file = getValue( request, "file", -1 );
  long repeat = getValue( request, "repeat", -2 );
  if( folder!=-1 && file!=-1 && repeat!=-2 ) {
    webPlayCommand.folder = folder;
    webPlayCommand.file = file;
    webPlayCommand.repeatTimes = repeat;
    webPlay = true;
  }

  request->redirect( "/" );
}

void onRequestHlasitost(AsyncWebServerRequest *request){
  asyncLogger.log( "@ req hlasitost" );
  
  long volume = getValue( request, "volume", -1 );
  if( volume>=1 && volume<=30 ) {
    hlasitost = volume;
    zmenaHlasitosti = true;
    config.setValue( "hlasitost", volume );
    saveConfigChange = true;
  }

  request->redirect( "/" );
}

void onRequestReset(AsyncWebServerRequest *request){
  asyncLogger.log( "@ req reset" );

  resetMp3 = true;

  request->redirect( "/" );
}



/**
ESP32 2.0.17

Using library Ticker at version 2.0.0 in folder: C:\Users\brouzda\AppData\Local\Arduino15\packages\esp32\hardware\esp32\2.0.17\libraries\Ticker 
Using library DFRobotDFPlayerMini at version 1.0.6 in folder: E:\dev.moje\arduino\libraries\DFRobotDFPlayerMini 
Using library DNSServer at version 2.0.0 in folder: C:\Users\brouzda\AppData\Local\Arduino15\packages\esp32\hardware\esp32\2.0.17\libraries\DNSServer 
Using library WiFi at version 2.0.0 in folder: C:\Users\brouzda\AppData\Local\Arduino15\packages\esp32\hardware\esp32\2.0.17\libraries\WiFi 
Using library Async TCP at version 3.4.0 in folder: E:\dev.moje\arduino\libraries\Async_TCP 
Using library ESP Async WebServer at version 3.7.7 in folder: E:\dev.moje\arduino\libraries\ESP_Async_WebServer 
Using library FS at version 2.0.0 in folder: C:\Users\brouzda\AppData\Local\Arduino15\packages\esp32\hardware\esp32\2.0.17\libraries\FS 
Using library ESP32AnalogRead at version 0.3.0 in folder: E:\dev.moje\arduino\libraries\ESP32AnalogRead 
Using library Tasker at version 2.0.3 in folder: E:\dev.moje\arduino\libraries\Tasker 
Using library SPIFFS at version 2.0.0 in folder: C:\Users\brouzda\AppData\Local\Arduino15\packages\esp32\hardware\esp32\2.0.17\libraries\SPIFFS 
 */