#include <DmxSimple.h>
#include <SPI.h>
#include <Ethernet.h>

// MAC and IP address for your Ethernet shield
byte mac[] = { 0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED };
IPAddress ip(192, 168, 1, 2);

// Create a server on port 80
EthernetServer server(80);

// DMX channels
const int c1 = 1;
const int c2 = 2;
const int c3 = 3;
const int c4 = 4;
const int c5 = 5;

// Current DMX values
int c1Val = 0;
int c2Val = 0;
int c3Val = 0;
int c4Val = 0;
int c5Val = 0;

// Buffer for incoming HTTP request
char request[256];
int reqIndex = 0;

void setup() {
  DmxSimple.usePin(3);
  DmxSimple.maxChannel(128);

  Ethernet.begin(mac, ip);
  server.begin();
}

void loop() {
  EthernetClient client = server.available();

  if (client) {
    reqIndex = 0;
    memset(request, 0, sizeof(request));

    unsigned long timeout = millis() + 1000; // 1 second timeout for client request

    while (client.connected() && millis() < timeout) {
      if (client.available()) {
        char c = client.read();
        if (reqIndex < (int)(sizeof(request) - 1)) {
          request[reqIndex++] = c;
        }

        // Check if end of HTTP headers reached
        if (strstr(request, "\r\n\r\n") != NULL) {
          break;
        }
      }
    }

    // Parse GET line: "GET /?c1Val=255&c2Val=0 HTTP/1.1"
    char* getStart = strstr(request, "GET ");
    if (getStart != NULL) {
      char* pathStart = getStart + 4; // Skip "GET "
      char* pathEnd = strstr(pathStart, " HTTP/");
      if (pathEnd != NULL) {
        *pathEnd = '\0';  // Null-terminate the path string

        // Ignore favicon.ico requests
        if (strncmp(pathStart, "/favicon.ico", 12) == 0) {
          client.stop();
          return;
        }

        // Check if path contains parameters starting with "/?"
        if (strncmp(pathStart, "/?", 2) == 0) {
          char* params = pathStart + 2; // Skip "/?"

          // Tokenize parameters separated by '&'
          char* param = strtok(params, "&");
          while (param != NULL) {
            if (strncmp(param, "c1Val=", 6) == 0) {
              int val = atoi(param + 6);
              c1Val = constrain(val, 0, 255);
              DmxSimple.write(c1, c1Val);
            }
            else if (strncmp(param, "c2Val=", 6) == 0) {
              int val = atoi(param + 6);
              c2Val = constrain(val, 0, 255);
              DmxSimple.write(c2, c2Val);
            }
            else if (strncmp(param, "c3Val=", 6) == 0) {
              int val = atoi(param + 6);
              c3Val = constrain(val, 0, 255);
              DmxSimple.write(c3, c3Val);
            }
            else if (strncmp(param, "c4Val=", 6) == 0) {
              int val = atoi(param + 6);
              c4Val = constrain(val, 0, 255);
              DmxSimple.write(c4, c4Val);
            }
            else if (strncmp(param, "c5Val=", 6) == 0) {
              int val = atoi(param + 6);
              c5Val = constrain(val, 0, 255);
              DmxSimple.write(c5, c5Val);
            }
            param = strtok(NULL, "&");
          }
        }
      }
    }

    // Send HTTP response (simple HTML form)
    client.println("HTTP/1.1 200 OK");
    client.println("Content-Type: text/html");
    client.println("Connection: close");
    client.println();

    client.println("<!DOCTYPE html><html><body>");
    client.println("<h1>Edit DMX Channels</h1>");
    client.println("<form action='/' method='GET'>");

    client.print("Channel 1: <input type='text' name='c1Val' value='");
    client.print(c1Val);
    client.println("'><br>");

    client.print("Channel 2: <input type='text' name='c2Val' value='");
    client.print(c2Val);
    client.println("'><br>");

    client.print("Channel 3: <input type='text' name='c3Val' value='");
    client.print(c3Val);
    client.println("'><br>");

    client.print("Channel 4: <input type='text' name='c4Val' value='");
    client.print(c4Val);
    client.println("'><br>");

    client.print("Channel 5: <input type='text' name='c5Val' value='");
    client.print(c5Val);
    client.println("'><br><br>");

    client.println("<input type='submit' value='Update'>");
    client.println("</form>");
    client.println("</body></html>");

    delay(1); // give client time to receive data
    client.stop();
  }
}
