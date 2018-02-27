/*
 Name:		ESP32_EEE423.ino
 Created:	2/12/2018 1:44:10 AM
 Author:	mohammed
*/

#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>

BLECharacteristic *pMassage;
std::string rxValue;

bool deviceConnected = false;
float txValue;
long int TiOffCaunt=0;
uint8_t Time_Off = int(0);
char BSt = 'z';
uint8_t device_time = 0;
const int readPin = 32; // Use GPIO number. See ESP32 board pinouts
const int LED = 2; // Could be different depending on the dev board. I used the DOIT ESP32 dev board.

				   // See the following for generating UUIDs:
				   // https://www.uuidgenerator.net/

#define DEVICE_SERVICE_UUID               "6E400040-B5A3-F393-E0A9-E50E24DCCA9E"// UART service UUID
#define DEVICE_CHARACTERISTIC_UUID_RX     "6E400041-B5A3-F393-E0A9-E50E24DCCA9E"
#define DEVICE_TIME_CHARACTERISTIC_UUID_RX"6E400042-B5A3-F393-E0A9-E50E24DCCA9E"

#define NOTIFY_SERVICE_UUID               "6E400060-B5A3-F393-E0A9-E50E24DCCA9E"
#define MASSAGE_CHARACTERISTIC_UUID_TX    "6E400061-B5A3-F393-E0A9-E50E24DCCA9E"


class MyServerCallbacks : public BLEServerCallbacks {
	void onConnect(BLEServer* pServer) {
		deviceConnected = true;
	};

	void onDisconnect(BLEServer* pServer) {
		deviceConnected = false;
	}
};

class DEVICE : public BLECharacteristicCallbacks {
	void onWrite(BLECharacteristic *pDevice) {
		rxValue = pDevice->getValue();

		if (rxValue.length() > 0) {
			Serial.println("*********");
			Serial.print("Received Value: ");

			for (int i = 0; i < rxValue.length(); i++) {
				Serial.print(rxValue[i]);
			}

			Serial.println();
			Serial.println("*********");
		}		
	}
};
class DEVICE_TIME : public BLECharacteristicCallbacks {
	void onWrite(BLECharacteristic *pDevice_Time) {
		std::string rxValue0 = pDevice_Time->getValue();
		if (rxValue0.length() > 0) {
			device_time = 0;
			for (int i = 0; i < (rxValue0.length() - 1); i++) {
				int power = rxValue0.length() - 2 - i;
				device_time = (int(rxValue0[i]) - 48)*pow(10, power) + device_time;
			}
			Serial.println(device_time);
			//device_time %= 2;
		}
	}
};

/* create a hardware timer */
hw_timer_t * timer = NULL;
long int TiCaunt = 0;

void IRAM_ATTR onTimer() {
	TiCaunt++;
	//Serial.println(TiCaunt);
}

void setup() {
	Serial.begin(115200);

	pinMode(LED, OUTPUT);

	// Create the BLE Device
	BLEDevice::init("JIC_EEE423_Project"); // Give it a name

										// Create the BLE Server
	BLEServer *pServer = BLEDevice::createServer();

	pServer->setCallbacks(new MyServerCallbacks());

	// Create the BLE Service
	BLEService *pDevice_Service = pServer->createService(DEVICE_SERVICE_UUID);
	BLEService *pNotify_Service = pServer->createService(NOTIFY_SERVICE_UUID);

	// Create a BLE Characteristic
	pMassage = pNotify_Service->createCharacteristic(
		MASSAGE_CHARACTERISTIC_UUID_TX,
		BLECharacteristic::PROPERTY_NOTIFY
	);

	pMassage->addDescriptor(new BLE2902());

	BLECharacteristic *pDevice = pDevice_Service->createCharacteristic(
		DEVICE_CHARACTERISTIC_UUID_RX,
		BLECharacteristic::PROPERTY_WRITE
	);
	BLECharacteristic *pDevice_Time = pDevice_Service->createCharacteristic(
		DEVICE_TIME_CHARACTERISTIC_UUID_RX,
		BLECharacteristic::PROPERTY_WRITE
	);

	pDevice->setCallbacks(new DEVICE());
	pDevice_Time->setCallbacks(new DEVICE_TIME());

	// Start the service
	pDevice_Service->start();
	pNotify_Service->start();

	// Start advertising
	pServer->getAdvertising()->start();
	Serial.println("Waiting a client connection to notify...");


	/* Use 1st timer of 4
	   1 tick take 1/(80MHZ/80) = 1us so we set divider 80 and count up */
	timer = timerBegin(0, 80, true);

	/* Attach onTimer function to our timer */
	timerAttachInterrupt(timer, &onTimer, true);

	/* Set alarm to call onTimer function every second 1 tick is 1us
	  => 1 second is 1000000us
	  Repeat the alarm (third parameter) */
	timerAlarmWrite(timer, 1000000, true);

	// Start an alarm 
	timerAlarmEnable(timer);
	Serial.println("start timer");
}

void loop() {
	if (deviceConnected) {
		// Some junk for now...
		// txValue = analogRead(readPin); // This could be an actual sensor reading
		
											   // Converting the value to a char array:
		char Time_Off_String[8];
		dtostrf(Time_Off, 3, 0, Time_Off_String); // val, min_width, digits_after_decimal, char_buffer
		Time_Off_String[3] = BSt;

		pMassage->setValue(Time_Off_String);

		pMassage->notify(); // proadcast the value through BLE
		/*Serial.print("*** Sent Value: ");
		Serial.print(Time_Off_String);
		Serial.println(" ***");
		Serial.print("*** Sent Value: ");
		Serial.print(device_time);
		Serial.println(" ***");*/
		delay(900);
	}
	else {
		delay(900);
	}
	// Do stuff based on the command received from BLE
	if (rxValue.find("A") != -1) {
		Serial.println("Turned ON!!");
		digitalWrite(LED, HIGH);
		if (device_time == 255) {
		}
		else {
			TiOffCaunt = int(device_time) + TiCaunt;
		}
		rxValue = "a";
		BSt = 'Z';
		delay(100);
	} 
	else if (rxValue.find("B") != -1 ) {
		Serial.println("Turned OFF!");
		digitalWrite(LED, LOW);
		rxValue = "a";
		BSt = 'z';
		delay(100);
	}

	if (device_time == 255) {
		delay(100);
	} else if (TiOffCaunt <= TiCaunt) {
		Serial.println("Turned OFF!");
		digitalWrite(LED, LOW);
		BSt = 'z';
		delay(100);
	}
	if (device_time != 255) {
		Time_Off = TiOffCaunt - TiCaunt;
		delay(100);
	}
	//Serial.println(TiOffCaunt);
}
