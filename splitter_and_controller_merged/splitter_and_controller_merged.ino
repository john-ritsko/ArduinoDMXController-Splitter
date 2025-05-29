#include <DmxSimple.h>
#include <SPI.h>
#include <Ethernet.h>

// MAC and IP for Ethernet Shield
byte mac[] = { 0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED };
IPAddress ip(192, 168, 1, 2);

EthernetServer server(80);

// DMX Channels
const int c1 = 1, c2 = 2, c3 = 3, c4 = 4, c5 = 5;
uint8_t c1Val = 0, c2Val = 0, c3Val = 0, c4Val = 0, c5Val = 0;

char requestBuffer[200];

bool inControllerMode = false;

int parseParam(const char *request, const char *key) {
  char *pos = strstr(request, key);
  if (pos) {
    pos += strlen(key);
    if (*pos == '=') {
      pos++;
      int val = 0;
      sscanf(pos, "%d", &val);
      if (val < 0) val = 0;
      else if (val > 255) val = 255;
      return val;
    }
  }
  return -1;
}

void setupController() {
  DmxSimple.usePin(3);
  DmxSimple.maxChannel(128);
  Ethernet.begin(mac, ip);
  server.begin();
}

void setup() {
  pinMode(2, INPUT);
  pinMode(LED_BUILTIN, OUTPUT);
  pinMode(4, OUTPUT);
  setupController();
}

void splitterLoop() {
  digitalWrite(LED_BUILTIN, LOW);
  digitalWrite(4, LOW);
}

void controllerLoop() {
  digitalWrite(LED_BUILTIN, HIGH);
  digitalWrite(4, HIGH);
    
  EthernetClient client = server.available();
  if (!client) return;

  int idx = 0;
  while (client.connected() && idx < sizeof(requestBuffer) - 1) {
    if (client.available()) {
      char c = client.read();
      requestBuffer[idx++] = c;
      if (idx >= 4 &&
          requestBuffer[idx - 4] == '\r' &&
          requestBuffer[idx - 3] == '\n' &&
          requestBuffer[idx - 2] == '\r' &&
          requestBuffer[idx - 1] == '\n') {
        break;
      }
    }
  }
  requestBuffer[idx] = '\0';

  char *getStart = strstr(requestBuffer, "GET ");
  if (!getStart) {
    client.stop();
    return;
  }
  getStart += 4;

  char *getEnd = strstr(getStart, " HTTP/");
  if (!getEnd) {
    client.stop();
    return;
  }
  *getEnd = '\0';

  if (strstr(getStart, "favicon.ico")) {
    client.stop();
    return;
  }

  char *query = strchr(getStart, '?');
  if (query) {
    query++;
    int v;
    v = parseParam(query, "c1Val");
    if (v != -1) { c1Val = (uint8_t)v; DmxSimple.write(c1, c1Val); }
    v = parseParam(query, "c2Val");
    if (v != -1) { c2Val = (uint8_t)v; DmxSimple.write(c2, c2Val); }
    v = parseParam(query, "c3Val");
    if (v != -1) { c3Val = (uint8_t)v; DmxSimple.write(c3, c3Val); }
    v = parseParam(query, "c4Val");
    if (v != -1) { c4Val = (uint8_t)v; DmxSimple.write(c4, c4Val); }
    v = parseParam(query, "c5Val");
    if (v != -1) { c5Val = (uint8_t)v; DmxSimple.write(c5, c5Val); }
  }

  client.println(F("HTTP/1.1 200 OK"));
  client.println(F("Content-Type: text/html"));
  client.println(F("Connection: close"));
  client.println();
  client.println(F("<!DOCTYPE html><html><body>"));
  client.println(F("<h1>Edit DMX Channels</h1>"));
  client.println(F("<form action='/' method='GET'>"));
  client.print(F("Channel 1: <input type='text' name='c1Val' value='")); client.print(c1Val); client.println(F("'><br>"));
  client.print(F("Channel 2: <input type='text' name='c2Val' value='")); client.print(c2Val); client.println(F("'><br>"));
  client.print(F("Channel 3: <input type='text' name='c3Val' value='")); client.print(c3Val); client.println(F("'><br>"));
  client.print(F("Channel 4: <input type='text' name='c4Val' value='")); client.print(c4Val); client.println(F("'><br>"));
  client.print(F("Channel 5: <input type='text' name='c5Val' value='")); client.print(c5Val); client.println(F("'><br><br>"));
  client.println(F("<input type='submit' value='Update'>"));
  client.println(F("</form></body></html>"));
  delay(1);
  client.stop();
}

void loop() {
  bool currentState = digitalRead(2);

  if (currentState) {
    controllerLoop();
  } else {
    splitterLoop();
  }
}
