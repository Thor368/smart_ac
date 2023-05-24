#include <ESP8266WiFi.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <IRrecv.h>
#include <DNSServer.h>
#include <ESP8266WebServer.h>
#include <WiFiManager.h>
#include <Ticker.h>

unsigned long tempm = 0;
String serialin;
float settemp = 22.0; //default setpoint after startup
int sleeptime = 0;
int tempmax = 30.0; //maximum setpoint of temp
int tempmin = 15.0; //minimum setpoint of temp
int maxsleeptime = 300; //maximum time of sleeptimer
const long temppoll = 3000; //refreshtime of tempsensors
int fan = 0; //fanlevel
bool drainstate = false; //drainstate
bool power = false; //powerstate
float hyst = 1; //hysterese
const int wls = 15;       //D0 BK war 16
const int oneWireBus = 5; //D1 OR
const int pump = 4;       //D2
const int cpump = 0;      //D3
uint16_t irreceiver = 2;  //D4 BL
const int comp = 14;      //D5
const int fanl = 12;      //D6
const int fanm = 13;      //D7
const int fanh = 16;      //D8 war 15
//RX GE, TX GN, A0 BR
OneWire oneWire(oneWireBus);
DallasTemperature sensors(&oneWire);
WiFiServer server(80);
String header;
bool web_powerstate = false;
String web_fanstate = "Off";
bool web_compstate = false;
uint8_t tempsensorin[8] = { 0x28, 0x99, 0x8E, 0x56, 0xB5, 0x01, 0x3C, 0xD1 }; //1st 1-wire sensor
uint8_t tempsensorout[8] = { 0x28, 0xF9, 0x17, 0x56, 0xB5, 0x01, 0x3C, 0x56 }; //7th 1-wire sensor
IRrecv irrecv(irreceiver);
decode_results results;
unsigned long currentTime = millis();
unsigned long previousTime = 0; 
const long timeoutTime = 2000;
Ticker componreq;
Ticker sleep;
Ticker dpump;
bool compressor = false;
bool lockout = true;
bool partymode = false;

void setup()
{
  compondelay();
	Serial.begin(9600);
  WiFiManager wifiManager;
  //wifiManager.resetSettings(); //Reset saved networks
  //wifiManager.autoConnect("LET Smart AC #1 Setup", "LET-S3tup!");
  //wifiManager.startConfigPortal("LET Smart AC #1 Setup", "LET-S3tup!");
  //wifiManager.setConfigPortalTimeout(60); //Timeout after code is beeing processed after no activity in the captive portal.
	sensors.begin();
	irrecv.enableIRIn();
	pinMode(comp, OUTPUT);
	pinMode(fanl, OUTPUT);
	pinMode(fanm, OUTPUT);
	pinMode(fanh, OUTPUT);
	pinMode(pump, OUTPUT);
  pinMode(cpump, OUTPUT);
	pinMode(wls, INPUT);
	digitalWrite(comp, HIGH);
	digitalWrite(pump, HIGH);
  digitalWrite(cpump, HIGH);
	digitalWrite(fanh, HIGH);
	digitalWrite(fanm, HIGH);
	digitalWrite(fanl, HIGH);
}

void loop()
{
  static bool wifi_ready = false;
  if ((WiFi.status() == WL_CONNECTED) && !wifi_ready)
  {
    wifi_ready = true;
    Serial.println(WiFi.localIP());
    server.begin();
    sleep.attach(60, sleeptimer);
  }

  //wifi
  if (WiFi.status() == WL_CONNECTED)
  {
    Serial.print("vis pwifi,1");
    ce(); 
  }
  if (WiFi.status() != WL_CONNECTED)
  {
    Serial.print("vis pwifi,0");
    ce();  
  }

  WiFiClient client = server.available();
  if (client)
  {
  String currentLine = "";
  currentTime = millis();
  previousTime = currentTime;
  while (client.connected() && currentTime - previousTime <= timeoutTime) // loop while the client's connected
  {
    currentTime = millis();         
    if (client.available())
    {
      char c = client.read();
      header += c;
      if (c == '\n')
      {
        if (currentLine.length() == 0)
        {
          client.println("HTTP/1.1 200 OK");
          client.println("Content-type:text/html");
          client.println("Connection: close");
          client.println();
          if (header.indexOf("GET /power/on") >= 0)
          {
            web_powerstate = true;
            power = true;
            Serial.print("vis ledpoff,0");
            ce();
            Serial.print("vis ledpon,1");
            ce();
            setfan();
            }
            else if (header.indexOf("GET /power/off") >= 0)
            {
              web_powerstate = false;
              web_fanstate = "Off";
              power = false;
              fan = 0;
              Serial.print("vis pfanlow,0");
              ce();
              Serial.print("vis pfanmed,0");
              ce();
              Serial.print("vis pfanhigh,0");
              ce();
              digitalWrite(fanh, HIGH);
              digitalWrite(fanm, HIGH);
              digitalWrite(fanl, HIGH);
              compressor = false;
              //drainstate = true;
              Serial.print("vis ledpoff,1");
              ce();
              Serial.print("vis ledpon,0");
              ce();
            }
            else if (header.indexOf("GET /fan/set") >= 0)
            {
              setfan();
            }
            else if (header.indexOf("GET /temp/up") >= 0)
            {
              settempup();
            }
            else if (header.indexOf("GET /temp/down") >= 0)
            {
              settempdown();
            }
            else if (header.indexOf("GET /sleep/up") >= 0)
            {
              setsleepup();
            }
            else if (header.indexOf("GET /sleep/down") >= 0)
            {
              setsleepdown();
            }
            float tempinweb = sensors.getTempC(tempsensorin);
            float tempoutweb = sensors.getTempC(tempsensorout);
            client.println("<!DOCTYPE html><html>");
            client.println("<head><title>LET Smart AC</title><meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">");
            client.println("<meta http-equiv=\"refresh\" content=\"10; URL=");
            client.println("http://");
            client.println(WiFi.localIP());
            client.println("\">");
            client.println("<link rel=\"icon\" href=\"http://andretibud.de/event/let_iot/smart_ac/favicon.ico\">");
            client.println("<link rel=\"shortcut icon\" href=\"http://andretibud.de/event/let_iot/smart_ac/favicon.ico\">");
            client.println("<link rel=\"shortcut icon\" href=\"http://andretibud.de/event/let_iot/smart_ac/favicon.png\" sizes=\"196x196\">");
            client.println("<link rel=\"icon\" type=\"image/png\" href=\"/http://andretibud.de/event/let_iot/smart_ac/faviconbig.png\" sizes=192x192>");
            client.println("<link rel=\"icon\" type=\"image/png\" href=\"http://andretibud.de/event/let_iot/smart_ac/favicon.png\" sizes=\"32x32\">");
            //client.println("<style>html {display: inline-block; margin: 0px auto; text-align: center;}");
            client.println("<style>h1 { font-family: Arial Black; display: inline-block; margin: 0px auto; text-align: center;}");
            client.println("text-decoration: none; font-size: 150px; margin: 2px; cursor: pointer;}");
            client.println("</style></head>");
            client.println("<body><div align=\"center\">");
            client.println("<img src=\"http://andretibud.de/event/let_iot/smart_ac/logo.png\" alt=\"LET_Logo\">");
            client.println("<h3>LET Smart AC #1</h3>");
            if (web_powerstate==false) {
              client.println("<a href=\"/power/on\"><img src=\"http://andretibud.de/event/let_iot/smart_ac/power.png\" alt=\"Power_Logo\"></a>");
              client.println("<img src=\"http://andretibud.de/event/let_iot/smart_ac/off.png\" alt=\"Power_off\"><br>");
            }
            if (web_powerstate==true) {
              client.println("<a href=\"/power/off\"><img src=\"http://andretibud.de/event/let_iot/smart_ac/power.png\" alt=\"Power_Logo\"></a>");
              client.println("<img src=\"http://andretibud.de/event/let_iot/smart_ac/on.png\" alt=\"Power_on\"><br>");
            }
            client.println("<img src=\"http://andretibud.de/event/let_iot/smart_ac/cool.png\" alt=\"Comp_Logo\">");
            if (web_compstate==false) {
              client.println("<img src=\"http://andretibud.de/event/let_iot/smart_ac/off.png\" alt=\"Power_off\">");
            }
            if (web_compstate==true) {
              client.println("<img src=\"http://andretibud.de/event/let_iot/smart_ac/on.png\" alt=\"Power_on\">");
            }
            client.println("<br><a href=\"/fan/set\"><img src=\"http://andretibud.de/event/let_iot/smart_ac/fan.png\" alt=\"Fan_Logo\"></a>");
            if (web_fanstate=="Off") {
            client.println("<img src=\"http://andretibud.de/event/let_iot/smart_ac/off.png\" alt=\"Fan_Off\">");
            }
            if (web_fanstate=="Low") {
            client.println("<img src=\"http://andretibud.de/event/let_iot/smart_ac/fan1.png\" alt=\"Fan_Low\">");
            }
            if (web_fanstate=="Med") {
            client.println("<img src=\"http://andretibud.de/event/let_iot/smart_ac/fan2.png\" alt=\"Fan_Med\">");
            }
            if (web_fanstate=="High") {
            client.println("<img src=\"http://andretibud.de/event/let_iot/smart_ac/fan3.png\" alt=\"Fan_High\">");
            }
            client.println("<br><img src=\"http://andretibud.de/event/let_iot/smart_ac/tout.png\" alt=\"Temp_out\"><h1>");
            if (tempoutweb == -127.00){
              client.println("---");
              client.println(" &#8451;");
            }
            else {
            client.println(tempoutweb,1);
            client.println(" &#8451;");
            }
            client.println("</h1><br>");
            client.println("<img src=\"http://andretibud.de/event/let_iot/smart_ac/tin.png\" alt=\"Temp_in\"><h1>");
            client.println(tempinweb,1);
            client.println(" &#8451;");
            client.println("</h1><br>");
            client.println("<img src=\"http://andretibud.de/event/let_iot/smart_ac/tset.png\" alt=\"Temp_set\"><br><h1>");
            client.println(settemp);
            client.println(" &#8451;");
            client.println("</h1><br>");
            client.println("<a href=\"/temp/up\"><img src=\"http://andretibud.de/event/let_iot/smart_ac/up.png\" alt=\"Temp_up\"></a>");
            client.println("<a href=\"/temp/down\"><img src=\"http://andretibud.de/event/let_iot/smart_ac/down.png\" alt=\"Temp_down\"></a>");
            client.println("<br>");
            client.println("<img src=\"http://andretibud.de/event/let_iot/smart_ac/sleep.png\" alt=\"Sleeptimer\"><br><h1>");
            client.println(sleeptime);
            client.println(" min");
            client.println("</h1><br>");
            client.println("<a href=\"/sleep/up\"><img src=\"http://andretibud.de/event/let_iot/smart_ac/up.png\" alt=\"Sleep_up\"></a>");
            client.println("<a href=\"/sleep/down\"><img src=\"http://andretibud.de/event/let_iot/smart_ac/down.png\" alt=\"Sleep_down\"></a>");
            client.println("</body></html>");
            client.println();
            break;
            } else {
            currentLine = "";
          }
        } else if (c != '\r') {
          currentLine += c;
        }
      }
    }
    header = "";
    client.stop();
  }
  //endwifi
	//drainpump
	if (drainstate == true)
	{
	  drain();
	}
	//end drainpump
	//display auto-dim
	int LDR = analogRead(A0);
	if (power == true)
  {
  	if (LDR >= 18)
  	{
  		Serial.print("dim=100");
  		ce();
  	}
  	if (LDR <= 18)
  	{
  		Serial.print("dim=1");
  		ce();
  	}
  }
  if (power == false)
  {
    Serial.print("dim=5");
    ce(); 
  }
	//end display auto-dim

	//if water sensor activates, turn on drain pump
	if (digitalRead(wls)== LOW)
	{
		//drainstate = true;
    digitalWrite(pump, HIGH);
	}
	//end drain pump

	//if ir code received
	if (irrecv.decode(&results))
	{
		unsigned int ircode = results.value;
		if(ircode==3674580118){
			settempup(); //+
		}
		else if(ircode==2882851182)
		{
			settempdown(); //-
		}
		else if(ircode==4140943054)
		{
			setpower(); //on-off
		}
		else if(ircode==1009394670)
		{
			setfan(); //fan
		}
		else if(ircode==1464959974)
		{
			setsleepup(); //t+
		}
		else if(ircode==3638877206)
		{
			setsleepdown(); //t-
		}
		irrecv.resume();
	}
	//end ir
  //compcontrol
  if (compressor == false)
  {
    digitalWrite(comp, HIGH);
    Serial.print("vis ledcon,0");
    ce();
    Serial.print("vis ledcoff,1");
    ce();
   web_compstate = false;
  }

  if (compressor && power && !lockout)
  {
    digitalWrite(comp, LOW);
    Serial.print("vis ledcoff,0");
    ce();
    Serial.print("vis ledcon,1");
    ce();
    web_compstate = true;
    if (!fan)
    {
      Serial.print("vis pfanlow,1");
      ce();
      Serial.print("vis pfanmed,0");
      ce();
      Serial.print("vis pfanhigh,0");
      ce();
      digitalWrite(fanh, HIGH);
      digitalWrite(fanm, HIGH);
      digitalWrite(fanl, LOW);
    }
  }
  //end compcontrol
	//read input from display
	serialin = Serial.readStringUntil('\r');
  //poll tempsensors
	unsigned long ctempm = millis();
	if ((ctempm - tempm) >= temppoll)
	{
		tempm = ctempm;
		sensors.requestTemperatures();
		gettempin(tempsensorin);
		gettempout(tempsensorout);
	}

	if(serialin.equals("refreshsettemp"))
	{
		Serial.print("tset.txt=\"");
		Serial.print(settemp,1);
    Serial.print("\"");
		ce();
	}
  if(serialin.equals("resetwifi"))
  {
    resetwifi();
  }
  if(serialin.equals("pmodeon"))
  {
    settemp = 5.00;
    partymode= true;
  }
  if(serialin.equals("pmodeoff"))
  {settemp = 22.00;
  partymode = false;
  }
	if(serialin.equals("refreshsleeptimer"))
	{
		Serial.print("tsleep.txt=\"");
		Serial.print(sleeptime);
		Serial.print("\"");
		ce();
	}
	if(serialin.equals("TU"))
	{
		settempup();
	}
	if(serialin.equals("TD"))
	{
		settempdown();
	}
	if(serialin.equals("SU"))
	{
		setsleepup();
	}
	if(serialin.equals("SD"))
	{
		setsleepdown();
	}
	if(serialin.equals("SF"))
	{
		setfan();
	}
	if(serialin.equals("PO"))
	{
		setpower();
	}
	if(serialin.equals("drain"))
	{
		drainstate = true;
	}
	if(serialin.equals("pmon"))
	{
  settemp = 5.0;
  fan = 3;
	}
 if(serialin.equals("pmoff"))
 {
  settemp = 20.0;
  fan = 1;
  }
  if(serialin.equals("refreshmain"))
  {
  refreshmain();
  }
	//end read from display
}

void refreshmain()
{
	if (power == false)
	{
		Serial.print("vis ledpoff,1");
		ce();
		Serial.print("vis ledpon,0");
		ce();
	}
	if (power == true)
	{
		Serial.print("vis ledpoff,0");
		ce();
		Serial.print("vis ledpon,1");
		ce();
	}
	if (comp == HIGH)
	{
		Serial.print("vis ledcon,0");
		ce();
		Serial.print("vis ledcoff,1");
		ce();
	}
	if (comp == LOW)
	{
		Serial.print("vis ledcon,1");
		ce();
		Serial.print("vis ledcoff,0");
		ce();
	}
	if (fan == 0)
	{
		Serial.print("vis pfanlow,0");
		ce();
		Serial.print("vis pfanmed,0");
		ce();
		Serial.print("vis pfanhigh,0");
		ce();
	}
	if (fan == 1)
	{
		Serial.print("vis pfanlow,1");
		ce();
		Serial.print("vis pfanmed,0");
		ce();
		Serial.print("vis pfanhigh,0");
		ce();
	}
	if (fan == 2)
	{
		Serial.print("vis pfanlow,0");
		ce();
		Serial.print("vis pfanmed,1");
		ce();
		Serial.print("vis pfanhigh,0");
		ce();
	}
	if (fan == 3)
	{
		Serial.print("vis pfanlow,0");
		ce();
		Serial.print("vis pfanmed,0");
		ce();
		Serial.print("vis pfanhigh,1");
		ce();
	}
}

void gettempin(DeviceAddress deviceAddress)
{
	float tempin = sensors.getTempC(deviceAddress);
	Serial.print("tin.txt=\"");
	Serial.print(tempin,1);
	Serial.print("\"");
	ce();
	Serial.print("tset.txt=\"");
	Serial.print(settemp,1);
	Serial.print("\"");
	ce();
	Serial.print("tsleep.txt=\"");
	Serial.print(sleeptime);
	Serial.print("\"");
	ce();
 Serial.print("twifiip.txt=\"");
 Serial.print (WiFi.localIP());
 Serial.print("\"");
 ce();
 Serial.print("qrwifiip.txt=\"");
 Serial.print ("http://");
 Serial.print (WiFi.localIP());
 Serial.print("\"");
 ce();
 if (power == false)
  {
    Serial.print("vis ledpoff,1");
    ce();
    Serial.print("vis ledpon,0");
    ce();
  }
  if (power == true)
  {
    Serial.print("vis ledpoff,0");
    ce();
    Serial.print("vis ledpon,1");
    ce();
  }
  if (comp == HIGH)
  {
    Serial.print("vis ledcon,0");
    ce();
    Serial.print("vis ledcoff,1");
    ce();
  }
  if (comp == LOW)
  {
    Serial.print("vis ledcon,1");
    ce();
    Serial.print("vis ledcoff,0");
    ce();
  }
  if (fan == 0)
  {
    Serial.print("vis pfanlow,0");
    ce();
    Serial.print("vis pfanmed,0");
    ce();
    Serial.print("vis pfanhigh,0");
    ce();
  }
  if (fan == 1)
  {
    Serial.print("vis pfanlow,1");
    ce();
    Serial.print("vis pfanmed,0");
    ce();
    Serial.print("vis pfanhigh,0");
    ce();
  }
  if (fan == 2)
  {
    Serial.print("vis pfanlow,0");
    ce();
    Serial.print("vis pfanmed,1");
    ce();
    Serial.print("vis pfanhigh,0");
    ce();
  }
  if (fan == 3)
  {
    Serial.print("vis pfanlow,0");
    ce();
    Serial.print("vis pfanmed,0");
    ce();
    Serial.print("vis pfanhigh,1");
    ce();
  }
 //tempcontrol
 if (settemp > tempin + hyst)
 {
    compressor = false; //Komp aus
    compondelay();
  }
  if (settemp < tempin - hyst)
  {
    if (power == true)
    {
    compressor = true; //Komp an
  }
  }
  //end tempcontrol
}

void gettempout(DeviceAddress deviceAddress)
{
	float tempout = sensors.getTempC(deviceAddress);

  if (!compressor)
    digitalWrite(cpump,HIGH);

  if (tempout == -127)
  {
    digitalWrite(cpump,HIGH);

    Serial.print("tout.txt=\"");
    Serial.print("---");
    Serial.print("\"");
    ce();
  }
  else if (tempout > -127)
  {
    if (compressor)
      digitalWrite(cpump, LOW);

  	Serial.print("tout.txt=\"");
  	Serial.print(tempout,1);
  	Serial.print("\"");
  	ce();
  }
}

void settempup()
{
	if (settemp + 1 < tempmax)
	{
		settemp = settemp + 1;
		Serial.print("tset.txt=\"");
		Serial.print(settemp,1);
		Serial.print("\"");
		ce();
	}
}

void settempdown()
{
	if (settemp - 1 > tempmin)
	{
		settemp = settemp - 1;
		Serial.print("tset.txt=\"");
		Serial.print(settemp,1);
		Serial.print("\"");
		ce();
	}

}

void setsleepup()
{
	if (sleeptime <= maxsleeptime){
		sleeptime = sleeptime + 15;
		Serial.print("tset.txt=\"");
		Serial.print(sleeptime);
		Serial.print("\"");
		ce();
	}
}

void setsleepdown()
{
	if (sleeptime == 0)
	{
	}
	else
	{
		sleeptime = sleeptime - 15;
		Serial.print("tset.txt=\"");
		Serial.print(sleeptime);
		Serial.print("\"");
		ce();
	}
}

void setpower()
{
	if (power == false)
	{
		power = true;
		Serial.print("vis ledpoff,0");
		ce();
		Serial.print("vis ledpon,1");
		ce();
    setfan();
    web_powerstate=true;
    drainstate=true;

	}
	else if (power == true)
	{
		power = false;
		fan = 0;
		Serial.print("vis pfanlow,0");
		ce();
		Serial.print("vis pfanmed,0");
		ce();
		Serial.print("vis pfanhigh,0");
		ce();
		digitalWrite(fanh, HIGH);
		digitalWrite(fanm, HIGH);
		digitalWrite(fanl, HIGH);
		compressor = false;
		Serial.print("vis ledpoff,1");
		ce();
		Serial.print("vis ledpon,0");
		ce();
   web_powerstate=false;
   drainstate = true;
	}
}

void setfan()
{
	if (power == true)
	{
		fan++;
		if (fan == 0)
		{
			Serial.print("vis pfanlow,0");
			ce();
			Serial.print("vis pfanmed,0");
			ce();
			Serial.print("vis pfanhigh,0");
			ce();
			digitalWrite(fanh, HIGH);
			digitalWrite(fanm, HIGH);
			digitalWrite(fanl, HIGH);
      web_fanstate = "Off";
		}
		if (fan == 1)
		{
			Serial.print("vis pfanlow,1");
			ce();
			Serial.print("vis pfanmed,0");
			ce();
			Serial.print("vis pfanhigh,0");
			ce();
			digitalWrite(fanh, HIGH);
			digitalWrite(fanm, HIGH);
			digitalWrite(fanl, LOW);
      web_fanstate = "Low";
		}
		if (fan == 2)
		{
			Serial.print("vis pfanlow,0");
			ce();
			Serial.print("vis pfanmed,1");
			ce();
			Serial.print("vis pfanhigh,0");
			ce();
			digitalWrite(fanh, HIGH);
			digitalWrite(fanl, HIGH);
			digitalWrite(fanm, LOW);
      web_fanstate = "Med";
		}
		if (fan == 3)
		{
			Serial.print("vis pfanlow,0");
			ce();
			Serial.print("vis pfanmed,0");
			ce();
			Serial.print("vis pfanhigh,1");
			ce();
			digitalWrite(fanl, HIGH);
			digitalWrite(fanm, HIGH);
			digitalWrite(fanh, LOW);
      web_fanstate = "High";
		}
		if (fan == 4)
		{
			Serial.print("vis pfanlow,1");
			ce();
			Serial.print("vis pfanmed,0");
			ce();
			Serial.print("vis pfanhigh,0");
			ce();
			digitalWrite(fanh, HIGH);
			digitalWrite(fanm, HIGH);
			digitalWrite(fanl, LOW);
			fan = 1;
      web_fanstate = "Low";
		}
	}
}

void ce()
{
	Serial.write(0xff);
	Serial.write(0xff);
	Serial.write(0xff);
}
void resetwifi()
{
  //wifiManager.resetSettings();
  WiFiManager wifiManager;
  wifiManager.autoConnect("LET Smart AC #1 Setup", "LET-S3tup!");
}
void compondelay()
{
  componreq.attach(120, compon); //2min Druckausgleich
}

void compon()
{
  componreq.detach();
  lockout = false;
}
void sleeptimer()
{
  if (sleeptime == 0)
  {
    
  }
  else if (sleeptime >= 1)
  {
    sleeptime --;
  }
  if (sleeptime == 1)
  { 
    setpower();
    sleeptime = 0;
  }
}
void drain()
{
      drainstate = false;
      digitalWrite(pump, LOW);
      Serial.print("vis ppump,1");
      ce();
      dpump.attach(8, drainpump);
}
 void drainpump()
    {
      digitalWrite(pump, HIGH);
      Serial.print("vis ppump,0");
      ce();
      dpump.detach();
    }
