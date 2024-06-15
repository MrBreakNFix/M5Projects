#include <WiFi.h>
#include <WebServer.h>
#include <M5StickCPlus2.h>

const char *ssid = "BreakDev";
const char *password = "30004ever";

WebServer server(80);

bool state = false;

const char *htmlPage = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
    <title>RC</title>
</head>
<body>
    <h1>RC</h1>
    <button onclick="rc()">{{BUTTON_TEXT}}</button>
    <script>
        function rc() {
            fetch('/rc').then(response => response.text()).then(state => {
                var button = document.querySelector('button');
                if (state === '1') {
                    button.textContent = 'ON';
                    console.log('ON');
                } else {
                    button.textContent = 'OFF';
                    console.log('OFF');
                }
            });
        }
    </script>
</body>
</html>
)rawliteral";


void handleRoot() {
    server.send(200, "text/html", htmlPage);
}

void handlerc() {
    state = !state;
    server.send(200, "text/plain", state ? "1" : "0");
}

void setup() {
    StickCP2.begin();

    StickCP2.Display.setRotation(1);
    StickCP2.Display.setTextColor(GREEN);
    StickCP2.Display.println("Rebooting ESP");
    StickCP2.Display.println("Starting AP");

    WiFi.softAP(ssid, password);

    server.on("/", handleRoot);
    server.on("/rc", handlerc);

    server.begin();
    StickCP2.Display.println("Ready");
}

void loop() {
    server.handleClient();

    // Speaker example
    if (state) {
        StickCP2.Speaker.tone(10000, 100);
        delay(10);
        StickCP2.Speaker.tone(4000, 20);
        delay(10);
    }
}
