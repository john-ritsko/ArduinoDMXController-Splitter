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

void setup() {
  DmxSimple.usePin(3);
  DmxSimple.maxChannel(128);
  Ethernet.begin(mac, ip);
  server.begin();
}

void loop() {
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

  if (strstr(getStart, "/update?")) {
    char *query = strchr(getStart, '?');
    if (query) {
      query++;
      int ch = parseParam(query, "ch");
      int val = parseParam(query, "val");
      if (ch >= 1 && ch <= 5 && val != -1) {
        switch (ch) {
          case 1: c1Val = val; DmxSimple.write(c1, c1Val); break;
          case 2: c2Val = val; DmxSimple.write(c2, c2Val); break;
          case 3: c3Val = val; DmxSimple.write(c3, c3Val); break;
          case 4: c4Val = val; DmxSimple.write(c4, c4Val); break;
          case 5: c5Val = val; DmxSimple.write(c5, c5Val); break;
        }
      }
    }
    client.println("HTTP/1.1 200 OK");
    client.println("Connection: close");
    client.println();
    client.stop();
    return;
  }

  // Serve UI
  client.println(F("HTTP/1.1 200 OK"));
  client.println(F("Content-Type: text/html"));
  client.println(F("Connection: close"));
  client.println();
  client.println(F("<!DOCTYPE html><html><head><style>"));
  client.println(F("body { background-color: black; color: white; font-family: sans-serif; text-align: center; }"));
  client.println(F(".slider-container {"));
  client.println(F("  display: flex;"));                  // Horizontal layout
  client.println(F("  justify-content: center;"));
  client.println(F("  gap: 40px;"));                      // Space between sliders
  client.println(F("  margin: 20px;"));
  client.println(F("}"));

  client.println(F(".slider-wrapper {"));
  client.println(F("  display: flex;"));
  client.println(F("  flex-direction: column;"));
  client.println(F("  align-items: center;"));
  client.println(F("  color: white;"));
  client.println(F("}"));

  client.println(F(".slider {"));
  client.println(F("  -webkit-appearance: none;"));
  client.println(F("  appearance: none;"));
  client.println(F("  width: 200px;"));                   // Becomes height
  client.println(F("  height: 8px;"));                    // Becomes width
  client.println(F("  background: #888;"));
  client.println(F("  border-radius: 4px;"));
  client.println(F("  outline: none;"));
  client.println(F("  transform: rotate(270deg);"));     // Rotate slider only
  client.println(F("  margin: 20px 0;"));
  client.println(F("}"));

  client.println(F(".slider::-webkit-slider-thumb {"));
  client.println(F("  -webkit-appearance: none;"));
  client.println(F("  appearance: none;"));
  client.println(F("  width: 16px;"));
  client.println(F("  height: 16px;"));
  client.println(F("  background: white;"));
  client.println(F("  border-radius: 50%;"));
  client.println(F("  cursor: pointer;"));
  client.println(F("}"));

  client.println(F("</style></head><body>"));  // Close head and open body
  client.println(F("<div class='slider-container'>")); // Open container


  for (int i = 1; i <= 5; i++) {
    client.print(F("<div class='slider-wrapper'>"));
    client.print(F("<div>Channel "));
    client.print(i);
    client.println(F("</div>"));
    client.print(F("<input type='range' class='slider' min='0' max='255' value='"));
    switch (i) {
      case 1: client.print(c1Val); break;
      case 2: client.print(c2Val); break;
      case 3: client.print(c3Val); break;
      case 4: client.print(c4Val); break;
      case 5: client.print(c5Val); break;
    }
    client.print(F("' oninput='sendUpdate("));
    client.print(i);
    client.println(F(", this.value)'>"));
    client.print(F("<div id='val"));
    client.print(i);
    client.print(F("'>"));
    switch (i) {
      case 1: client.print(c1Val); break;
      case 2: client.print(c2Val); break;
      case 3: client.print(c3Val); break;
      case 4: client.print(c4Val); break;
      case 5: client.print(c5Val); break;
    }
    client.println(F("</div></div>"));

  }
  client.println(F("</div>"));  // Close slider-container

  client.println(F("</div>"));


  client.println(F("<script>"));
  client.println(F("function sendUpdate(channel, value) {"));
  client.println(F("  var xhr = new XMLHttpRequest();"));
  client.println(F("  xhr.open('GET', '/update?ch=' + channel + '&val=' + value, true);"));
  client.println(F("  xhr.send();"));
  client.println(F("  document.getElementById('val' + channel).textContent = value;"));
  client.println(F("}"));
  client.println(F("</script>"));


  client.println(F("</body></html>"));
  delay(1);
  client.stop();
}