#include <DmxSimple.h>
#include <SPI.h>
#include <Ethernet.h>

byte mac[] = { 0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED };
IPAddress ip(192, 168, 1, 2);

EthernetServer server(80);

int c1 = 1;
int c2 = 2;
int c3 = 3;
int c4 = 4;
int c5 = 5;

int c1Val = 0;
int c2Val = 0;
int c3Val = 0;
int c4Val = 0;
int c5Val = 0;

int parseVal(String &req, String key) {
  int i = req.indexOf(key + "=");
  if (i != -1) {
    int start = i + key.length() + 1;
    int end = req.indexOf('&', start);
    if (end == -1) end = req.length();
    String val = req.substring(start, end);
    if (val.length() > 0) {
      int v = val.toInt();
      if (v > 255) return 255;
      if (v < 0) return 0;
      return v;
    }
  }
  return -1;
}

void setup() {
  DmxSimple.usePin(3);
  DmxSimple.maxChannel(128);
  Ethernet.begin(mac, ip);
  server.begin();
}

void loop() {
  EthernetClient client = server.available();
if (client) {
  boolean currentLineIsBlank = true;
  String requestLine = "";

  while (client.connected()) {
    if (client.available()) {
      requestLine = client.readStringUntil('\r'); // Read full GET line
      int v;


      v = parseVal(requestLine, "c1Val");
        if (v != -1) {
          c1Val = v;
          DmxSimple.write(c1, c1Val);
        }

      v = parseVal(requestLine, "c2Val");
        if (v != -1) {
          c2Val = v;
          DmxSimple.write(c2, c2Val);
        }

      v = parseVal(requestLine, "c3Val");
        if (v != -1) {
          c3Val = v;
          DmxSimple.write(c3, c3Val);
        }

      v = parseVal(requestLine, "c4Val");
        if (v != -1) {
          c4Val = v;
          DmxSimple.write(c4, c4Val);
        }
      
      v = parseVal(requestLine, "c5Val");
        if (v != -1) {
          c5Val = v;
          DmxSimple.write(c5, c5Val);
        }

      // Respond with form
      String response = "<html><body>";
      response += "<h1>Edit DMX Channels</h1>";
      response += "<form action='/' method='GET'>";
      response += "Channel 1: <input type='text' name='c1Val' value='" + String(c1Val) + "'><br>";
      response += "Channel 2: <input type='text' name='c2Val' value='" + String(c2Val) + "'><br>";
      response += "Channel 3: <input type='text' name='c3Val' value='" + String(c3Val) + "'><br>";
      response += "Channel 4: <input type='text' name='c4Val' value='" + String(c4Val) + "'><br>";
      response += "Channel 5: <input type='text' name='c5Val' value='" + String(c5Val) + "'><br><br>";
      response += "<input type='submit' value='Update'>";
      response += "</form>";
      response += "</body></html>";

      client.println("HTTP/1.1 200 OK");
      client.println("Content-Type: text/html");
      client.print("Content-Length: ");
      client.println(response.length());
      client.println("Connection: close");
      client.println();
      client.println(response);

      break; // Done with response
    }
  }
  delay(1);
  client.stop();
}

}
