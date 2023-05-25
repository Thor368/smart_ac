#include "config.h"

#include <ESP8266WiFi.h>
#include <DNSServer.h>
#include <ESP8266WebServer.h>

WiFiServer server(80);
String header;
bool web_powerstate = false;
String web_fanstate = "Off";
bool web_compstate = false;

void web_update(void)
{
  static bool wifi_ready = false;
  if ((WiFi.status() == WL_CONNECTED) && !wifi_ready)
  {
    wifi_ready = true;
    Serial.println(WiFi.localIP());
    server.begin();
  }

   WiFiClient client = server.available();
  if (client)
  {
    String currentLine = "";
    currentTime = millis();
    previousTime = currentTime;
    while (client.connected() && ((currentTime - previousTime) <= timeoutTime)) // loop while the client's connected
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
            client.println("Connection: close\n");
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
