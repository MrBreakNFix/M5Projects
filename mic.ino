#include <WiFi.h>
#include <WebServer.h>
#include <WebSocketsServer.h>
#include <M5StickCPlus2.h>

WebServer server(80);
WebSocketsServer webSocket = WebSocketsServer(81);

const char *htmlPage = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
    <title>M5StickCPlus2 audio stream</title>
</head>
<body>
    <audio id="audio" controls autoplay></audio>
    <script>
        var audio = document.getElementById('audio');
        var audioContext = new (window.AudioContext || window.webkitAudioContext)();
        var scriptNode = audioContext.createScriptProcessor(4096, 1, 1);
        var websocket = new WebSocket('ws://' + location.hostname + ':81/');
        
        websocket.binaryType = 'arraybuffer';
        
        websocket.onmessage = function(event) {
            var arrayBuffer = event.data;
            var audioBuffer = new Int16Array(arrayBuffer);
            var float32Array = new Float32Array(audioBuffer.length);
            for (var i = 0; i < audioBuffer.length; i++) {
                float32Array[i] = audioBuffer[i] / 32768.0; // Convert to [-1, 1]
            }
            var buffer = audioContext.createBuffer(1, float32Array.length, 44100);
            buffer.copyToChannel(float32Array, 0);
            var source = audioContext.createBufferSource();
            source.buffer = buffer;
            source.connect(audioContext.destination);
            source.start();
        };
    </script>
</body>
</html>
)rawliteral";

void handleRoot() {
    server.send(200, "text/html", htmlPage);
}

void sendAudioData() {
    static const size_t bufferSize = 512;
    static int16_t audioBuffer[bufferSize];
    
    if (StickCP2.Mic.record(audioBuffer, bufferSize, 44100)) {
        webSocket.broadcastBIN((uint8_t*)audioBuffer, bufferSize * sizeof(int16_t));
    }
}

void onWebSocketEvent(uint8_t num, WStype_t type, uint8_t *payload, size_t length) {
    switch(type) {
        case WStype_DISCONNECTED:
            Serial.printf("[%u] Disconnected!\n", num);
            break;
        case WStype_CONNECTED:
            {
                IPAddress ip = webSocket.remoteIP(num);
                Serial.printf("[%u] Connection from %s\n", num, ip.toString().c_str());
            }
            break;
        case WStype_TEXT:
            break;
        case WStype_BIN:
            break;
        case WStype_ERROR:
        case WStype_FRAGMENT_TEXT_START:
        case WStype_FRAGMENT_BIN_START:
        case WStype_FRAGMENT:
        case WStype_FRAGMENT_FIN:
            break;
    }
}

void setup() {
    StickCP2.begin();
    StickCP2.Display.setRotation(1);
    StickCP2.Display.setTextColor(GREEN);
    StickCP2.Display.println("Setting up Hotspot...");

    WiFi.softAP("BreakDev", "3k4ever");
    IPAddress IP = WiFi.softAPIP();

    server.on("/", handleRoot);
    server.begin();
    StickCP2.Display.println("Hotspot Ready");

    Serial.begin(115200);
    Serial.println();
    Serial.println("Hotspot Ready");

    StickCP2.Mic.begin();

    webSocket.begin();
    webSocket.onEvent(onWebSocketEvent);

    Serial.println("WebSocket Ready");
    StickCP2.Display.println("WebSocket Ready");
}

void loop() {
    server.handleClient();
    webSocket.loop();

    if (webSocket.connectedClients() > 0) {
        sendAudioData();
    }
}
