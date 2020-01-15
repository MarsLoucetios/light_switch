#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <Bounce2.h>
#include <EEPROM.h>

const char* ssid = "ssid";
const char* password = "password";
const char* mqtt_server = "ip-address";

WiFiClient switch_light;
PubSubClient client(switch_light);
long lastMsg = 0;
char msg[50];
int value = 0;

const char* outTopic = "home/light/sub";
const char* inTopic = "home/light/pub";

int relay_pin = D7;
int button_pin = D3;
int wifi_led_pin = D1;
bool relayState = LOW;

Bounce debouncer = Bounce();

void setup_wifi() {

	delay(10);
	Serial.println();
	Serial.print("Connecting to ");
	Serial.println(ssid);

	WiFi.begin(ssid, password);

	while (WiFi.status() != WL_CONNECTED) {
		int extButton();
		for (int i = 0; i<500; i++) {
			extButton();
			delay(1);
		}
		Serial.print(".");
	}
	digitalWrite(wifi_led_pin, LOW);
	delay(500);
	digitalWrite(wifi_led_pin, HIGH);
	delay(500);
	digitalWrite(wifi_led_pin, LOW);
	delay(500);
	digitalWrite(wifi_led_pin, HIGH);
	Serial.println("");
	Serial.println("WiFi connected");
	Serial.println("IP address: ");
	Serial.println(WiFi.localIP());
}

void callback(char* topic, byte* payload, unsigned int length) {
	Serial.print("Message arrived [");
	Serial.print(topic);
	Serial.print("] ");
	for (int i = 0; i < length; i++) {
		Serial.print((char)payload[i]);
	}
	Serial.println();

	if ((char)payload[0] == '0') {
		digitalWrite(relay_pin, LOW);
		client.publish(outTopic, "0");
		Serial.println("relay_pin -> LOW");
		relayState = LOW;
		EEPROM.write(0, relayState);
		EEPROM.commit();
	}
	else if ((char)payload[0] == '1') {
		digitalWrite(relay_pin, HIGH);
		client.publish(outTopic, "1");
		Serial.println("relay_pin -> HIGH");
		relayState = HIGH;
		EEPROM.write(0, relayState);
		EEPROM.commit();
	}
	else if ((char)payload[0] == '2') {
		relayState = !relayState;
		digitalWrite(relay_pin, relayState);
		Serial.print("relay_pin -> switched to ");
		Serial.println(relayState);
		EEPROM.write(0, relayState);
		EEPROM.commit();
	}
}

void reconnect() {
	while (!client.connected()) {
		Serial.print("Attempting MQTT connection...");
		if (client.connect("ESP8266")) {
			Serial.println("connected");
			client.subscribe(inTopic);
		}
		else {
			Serial.print("failed, rc=");
			Serial.print(client.state());
			Serial.println(" try again in 5 seconds");
			for (int i = 0; i<5000; i++) {
				int extButton();
				delay(1);
			}
		}
	}
}

void extButton() {
	debouncer.update();

	if (debouncer.fell()) {
		Serial.println("Debouncer fell");
		relayState = !relayState;
		digitalWrite(relay_pin, relayState);
		EEPROM.write(0, relayState);
		EEPROM.commit();
		if (relayState == 1) {
			client.publish(outTopic, "1");
		}
		else if (relayState == 0) {
			client.publish(outTopic, "0");
		}
	}
}

void setup() {
	EEPROM.begin(512);
	pinMode(relay_pin, OUTPUT);
	pinMode(button_pin, INPUT_PULLUP);
	pinMode(wifi_led_pin, OUTPUT);
	relayState = EEPROM.read(0);
	digitalWrite(relay_pin, relayState);

	debouncer.attach(button_pin);
	debouncer.interval(50);

	digitalWrite(wifi_led_pin, LOW);
	delay(500);
	digitalWrite(wifi_led_pin, HIGH);
	delay(500);

	Serial.begin(115200);
	setup_wifi();
	client.setServer(mqtt_server, 1883);
	client.setCallback(callback);
}

void loop() {

	if (!client.connected()) {
		reconnect();
	}
	client.loop();
	extButton();
}
