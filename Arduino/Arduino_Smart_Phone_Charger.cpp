/*
 * Based on original sketch:
 * Bluetooth HC-06/05 (SLAVE) control from your Android phone RSB May 2016
 */
#include "Arduino_Smart_Phone_Charger.h"
#include "Arduino.h"
#include <SoftwareSerial.h>
#include <stdlib.h> // required for atoi

#include <wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

// If you are using an HC06 set the following line to false
#define USING_HC05 true

// Instantiate our BT object. First value is RX pin, second value TX pin
// NOTE: do NOT connect the RX directly to the Arduino unless you are using a
// 3.3v board. In all other cases connect pin 4 to a 1K2 / 2K2 resistor divider
/*
 * ---- pin 6-----> |----1K2----| to HC06 RX |----2K2----| -----> GND
 *
 * See my video #36 & #37 at www.youtube.com/RalphBacon for more details.
 */
// Communicate with the BT device on software controlled serial pins
// So the TX pin on the HC0X device goes to pin 4, the RX pin here.
SoftwareSerial BTserial(4, 6); // RX , TX

// Our BT Serial Buffer where BT sends its text data to the Arduino
char buffer[19] = { '\0' };

// Get the flag to show whether the phone thinks it is plugged in
bool isPluggedIn = false;

// Conversion buffer for the individual 3-digit integer values
char btValue[4] = { 0 };

// Heartbeat buffer (string) will contain "HEARTBEATn"
char heartBeat[11] = { '\0' };
unsigned long lastHeartBeat = 0;

// Connected? Only works on HC-05
#define connectedState 10
bool prevStateDisconnected = true;

// Power on/off MOSFET and LED
#define pwrControlLED 9

// Connected LED
#define connectedLED 3

// Heartbeat LED (must be PWM if you want it to fade in/out)
#define heartbeatLED 5

// Are we charging to the MAX ppint or discharging to MIN level?
// Note this depends on whehter the output is HIGH or LOW in Setup()
bool chargingUp = true;
int batLevel = 0;

// Rolling average for current consumption (due to phone jitter)
uint16_t mA_Average[20] = { 0 };
uint8_t mAIdx = 0;

// If we have never started charging pre-fill the above array on first charge
bool firstCharge = true;

// I2C address of the INA219 device (can be changed by soldering board)
byte response, hexAddress = 0x40;

// SSD1306 OLED Display
//#define SCREEN_WIDTH 128 // OLED display width, in pixels
//#define SCREEN_HEIGHT 32 // OLED display height, in pixels
//Adafruit_SSD1306 display(0);

// New method of invoking the SSD1306 object
Adafruit_SSD1306 display(128, 32);

// Forward declarations
void printDateTimeStamp(char buffer[15]);
void displayBTbuffer();
bool extractHeartBeatFromBTdata(int startIdx, int endIdx);
int extractDataFromBTdata(int startIdx, int endIdx);
void printRawData();
void pluggedInStatus();
void displayBatteryPercent();

// SSD1306 OLED
void displayHeartBeat();
void displayChargeStatus(bool charging = true);

// INA219 Current Monitor
void INA219_setup();
int getMilliAmps();

// DEBUG flag controls whether we send Serial.print statements
// #define DEBUGMSG 1

// -----------------------------------------------------------------------------------
// SET UP   SET UP   SET UP   SET UP   SET UP   SET UP   SET UP   SET UP   SET UP
// -----------------------------------------------------------------------------------
void setup() {

	// Power charging pin LED
	pinMode(pwrControlLED, OUTPUT);
	digitalWrite(pwrControlLED, HIGH);

	// Connected LED
	pinMode(connectedLED, OUTPUT);
	digitalWrite(connectedLED, LOW);

	// Heartbeat LED (fades in/out on PMW)
	pinMode(heartbeatLED, OUTPUT);
	digitalWrite(heartbeatLED, LOW);

	// State pin (is Bluetooth device connected according to HC05?)
	pinMode(connectedState, INPUT);

	// Serial Windows stuff
	Serial.begin(9600);

	// Set baud rate of HC-06 that you set up using the FTDI USB-to-Serial module
	BTserial.begin(9600);

	// Ensure we don't wait forever for a BT character
	BTserial.setTimeout(2000); //mS

	// INA219 Current Monitor initialisation
	INA219_setup();

	// SSD1306 initialize with the I2C addr 0x3C (for the 128x32)
	if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C))
	{
#ifdef DEBUGMSG
		Serial.println(F("SSD1306 allocation failed"));
#endif
		for (;;)
			; // Don't proceed, loop forever
	}

	// Clear the buffer.
	display.clearDisplay();
	display.display();
	display.cp437(true);         // Use full 256 char 'Code Page 437' font

#ifdef DEBUGMSG
	display.setTextSize(1);      // Normal 1:1 pixel scale
	display.setTextColor(SSD1306_WHITE); // Draw white text
	display.setCursor(0, 0);     // x,y Start at top-left corner

	// Not all the characters will fit on the display. This is normal.
	// Library will draw what it can and the rest will be clipped.

	for (int16_t i = 0; i < 256; i++)
	{
		if (i == '\n') display.write(' ');
		else
		display.write(i);
	}

	display.display();
#endif

	displayNotConnected();
	delay(2000);

	// Setup done
#ifdef DEBUGMSG
	Serial.println(F("Set up complete"));
#endif
}

// Main process to inspect the BT data from phone
void processBTdata()
{
	// Get the chars. This blocks until the number specified
	// has been read or it times out
	int byteCount = BTserial.readBytes(buffer, 18);

	// Useful to see the raw BT buffer in debugging
	printRawData();

	// If we timed out we won't have the full X bytes
	if (byteCount == 18)
	{
		// first 8 chars are time format "hh:MM:ss"
		printDateTimeStamp(buffer);

		// Flag for whether phone is currently plugged in
		isPluggedIn = buffer[17] == '1';

		// Is this the heartbeat?
		if (extractHeartBeatFromBTdata(8, 16))
		{
#ifdef DEBUGMSG
			Serial.println(F("Heartbeat received."));
#endif
			pluggedInStatus();

			// SSD1306 show the heart symbol
			displayHeartBeat();

			// Reset the heartbeat clock so we don't get a warning
			lastHeartBeat = millis();
		} else
		{
			/* This next section extracts the integer values from
			 * the text data, and displays lots of debugging info
			 * so we can track what is going on whilst developing
			 * this program!
			 */
			batLevel = extractDataFromBTdata(8, 10);
#ifdef DEBUGMSG
			Serial.print(F("Battery Level:"));
			Serial.print(batLevel);
#endif
			displayBTbuffer();

			printDateTimeStamp(buffer);

			int maxCharge = extractDataFromBTdata(11, 13);
#ifdef DEBUGMSG
			Serial.print(F("Max Charge Level:"));
			Serial.print(maxCharge);
#endif
			displayBTbuffer();

			printDateTimeStamp(buffer);

			int minCharge = extractDataFromBTdata(14, 16);
#ifdef DEBUGMSG
			Serial.print(F("Min charge Level:"));
			Serial.print(minCharge);
#endif
			displayBTbuffer();

			printDateTimeStamp(buffer);

#ifdef DEBUGMSG
			Serial.print(F("Phone plugged in:"));

			if (isPluggedIn)
			{
				Serial.println(F("Yes"));
			} else
			{
				Serial.println(F("No"));
			}
#endif

			// If the battery is now >= max wanted, switch off
			if (batLevel >= maxCharge && chargingUp)
			{
				digitalWrite(pwrControlLED, LOW);
				chargingUp = false;
			}

			// If the battery is now <= min wanted, switch on
			if (batLevel <= minCharge && !chargingUp)
			{
				digitalWrite(pwrControlLED, HIGH);
				chargingUp = true;
				delay(2250); // give time for charging to start
			}

			printDateTimeStamp(buffer);
			displayBatteryPercent();

			// Is phone plugged in when it should be?
			pluggedInStatus();

			// Data in lieu of heartbeat still counts as heartbeat
			lastHeartBeat = millis();
		}
	} else
	{
#ifdef DEBUGMSG
		Serial.print(F("Only received "));
		Serial.print(byteCount);
		Serial.println(F(" characters - ignored."));
#endif
	}

	// Discard partial data in serial buffer
	while (BTserial.available())
	{
		BTserial.read();
#ifdef DEBUGMSG
		Serial.println(F("Discarded a serial character."));
#endif
	}

}

// -----------------------------------------------------------------------------------
// MAIN LOOP     MAIN LOOP     MAIN LOOP     MAIN LOOP     MAIN LOOP     MAIN LOOP
// -----------------------------------------------------------------------------------
void loop() {
	/* If the HC-06/05 has some data for us, get it.
	 *
	 * First 8 bytes time in text format hh:MM:ss
	 *
	 * Then follows three variables of 3 bytes each in text format
	 * that contain the current battery level, the level at which the
	 * user wants to start charging and the level the user wants to
	 * stop charging
	 *
	 * The final byte is whether the phone is currently plugged in.
	 *
	 * If there has been no change to the battery level then a
	 * heartbeat is sent just so that this sketch knows the phone
	 * is still sending data down the line
	 */

#if USING_HC05
	// Only if connected do any of this (assume HC06 always connected)
	if (digitalRead(connectedState))
	#else
	// When using HC06 cannot detected whether connected
	if(true)
#endif
	{
		// Confirm connected state if previously disconnected
		if (prevStateDisconnected)
		{
			Serial.println(F("CONNECTED."));

			// TODO move to function
			// Display the heart
			display.setTextSize(2);
			display.setTextColor(SSD1306_WHITE);
			display.setCursor(0, 15);     // x,y Start at top-left corner
			display.print("CONNECTED");
			display.display();

			prevStateDisconnected = false;
			// TODO Turn on the (blue) connected LED here
		}

		// If we have serial data to process
		if (BTserial.available())
		{
			// Get the chars. This blocks until the number specified
			// has been read or it times out
			processBTdata();
		} else
		{
			// We ARE connected but no BT data to process at this time.

			// Update charge current on screen
			//if (chargingUp)
			//{
			displayHeartBeat();

			// Toggle between charge current and battery level
			static int displayCharge = 0;

			if (++displayCharge > 10)
			{
				displayCharge = 0;
				displayBatteryPercent();
			}
			else
			{
				displayCharge++;
				displayChargeStatus(chargingUp);
			}
			//}

			// Check last heartbeat
			// See http://www.gammon.com.au/millis on why we do it this way
			if (millis() - lastHeartBeat >= 300000UL)
			{
#ifdef DEBUGMSG
				Serial.println(F("Connected, but no heartbeat for 5 minutes"));
#endif
				lastHeartBeat = millis();

				// TODO Do something with the heartbeat LED here
			}
		}
	}
	else
	{
		// If we were previously connected but now are not
		if (!prevStateDisconnected)
		{
#ifdef DEBUGMSG
			Serial.println(F("NOT CONNECTED."));
#endif
			prevStateDisconnected = true;

			// TODO Do something with the connected LED here
			displayNotConnected();
		}
	}

	// Give the data a chance to arrive
	delay(100);
}

// Print the first 8 characters - always the timestamp
void printDateTimeStamp(char buffer[15]) {
	static char dummy __attribute__ ((used)) = buffer[1];
#ifdef DEBUGMSG
	for (auto cnt = 0; cnt < 8; cnt++)
	{
		Serial.print((char) buffer[cnt]);
	}
	Serial.print(" ");
#endif
}

// Display the extracted 3-character value buffer
void displayBTbuffer()
{
#ifdef DEBUGMSG
	Serial.print(" (");
	for (auto cnt = 0; cnt < 3; cnt++)
	{
		Serial.print(btValue[cnt]);
	}
	Serial.println(")");
#endif
}

// Extract the data element to check for heartbeat string
bool extractHeartBeatFromBTdata(int startIdx, int endIdx)
		{

	int hbIdx = 0;
	memset(heartBeat, '\0', sizeof(heartBeat));
	for (auto cnt = startIdx; cnt <= endIdx; cnt++)
	{
		heartBeat[hbIdx++] = buffer[cnt];
	}

//	Serial.print("Heartbeat buffer:");
//	for (auto cnt = 0; cnt < 9; cnt++)
//	{
//		Serial.print(heartBeat[cnt]);
//	}

	char key[] = "HEARTBEAT";
	if (strcmp(heartBeat, key) == 0)
	{
		return true;
	} else
	{
		return false;
	}
}

// Extract the 3-digit value from the string BT data
int extractDataFromBTdata(int startIdx, int endIdx)
		{

	// Clear the target buffer and ensure last (4th) char
	// is a null terminator 0
	memset(btValue, 0, sizeof(btValue));

	int btValueIdx = 0;
	for (auto cnt = startIdx; cnt <= endIdx; cnt++)
	{
		btValue[btValueIdx++] = buffer[cnt];
	}

	// Convert the 'string' to an integer
	int returnValue = atoi(btValue);
	return returnValue;
}

// What's coming in the BT serial buffer?
void printRawData()
{
#ifdef DEBUGMSG
	Serial.print(F("Raw buffer: "));
	for (auto cnt = 0; cnt < 18; cnt++)
	{
		Serial.print(buffer[cnt]);
	}
	Serial.println(" ");
#endif
}

// Is phone connected to USB power source?
void pluggedInStatus()
{
	// Phone not plugged in?
	if (!isPluggedIn && chargingUp)
	{
		printDateTimeStamp(buffer);
#ifdef DEBUGMSG
		Serial.println(F("Plug phone in to Charge."));
#endif
	}
}

// Display beating heart symbol indicating HeartBeat is active
void displayHeartBeat()
{
	// TODO beating heart
	const int startPos = 8;

	// Display the heart
	display.setTextSize(2);
	display.setTextColor(SSD1306_WHITE);
	display.setCursor(0, startPos);	// x,y Start at top-left corner
	display.write(3);
	display.display();
	delay(200);

	// Clear the heart
	display.fillRect(0, 0, 10, display.height(), SSD1306_BLACK);
	display.display();
	delay(200);

	// Display the heart
	display.setTextSize(2);
	display.setTextColor(SSD1306_WHITE);
	display.setCursor(0, startPos);	// x,y Start at top-left corner
	display.write(3);
	display.display();
}

void INA219_setup()
{
	// Initialise I2C (default address of 0x40)
	Wire.begin();

	// Test that we can communicate with the device
	Wire.beginTransmission(hexAddress);

	// Set the calibration to 32V @2A by writing two bytes (4096, an int) to that register
	Wire.write(INA219_REG_CALIBRATION);
	Wire.write((4096 >> 8) & 0xFF);
	Wire.write(4096 & 0xFF);

	Wire.endTransmission();

	// Config
	Wire.beginTransmission(hexAddress);
	Wire.write(INA219_REG_CONFIG);

	// Set Config register stating we want:
	uint16_t config = INA219_CONFIG_BVOLTAGERANGE_32V   // 32 volt, 2A range
						| INA219_CONFIG_GAIN_8_320MV   // 8 x Gain
						| INA219_CONFIG_BADCRES_12BIT   // 12-bit bus ADC resolution
						| INA219_CONFIG_SADCRES_12BIT_8S_4260US   // number of averaged samples
						| INA219_CONFIG_MODE_SANDBVOLT_CONTINUOUS;   // Continuouis conversion
	Wire.write((config >> 8) & 0xFF);
	Wire.write(config & 0xFF);

	// See if something acknowledged the transmission
	response = Wire.endTransmission();
	if (response == 0)
	{
#ifdef DEBUGMSG
		Serial.print(F("I2C device found at hexAddress 0x"));
		if (hexAddress < 16)
		Serial.print("0");
		Serial.println(hexAddress, HEX);
#endif
	}
	else if (response == 4) // unknown error
	{
#ifdef DEBUGMSG
		Serial.print(F("Unknown response at hexAddress 0x"));
		if (hexAddress < 16)
		Serial.print("0");
		Serial.println(hexAddress, HEX);
#endif
	}

	// All done here
#ifdef DEBUGMSG
	Serial.println(F("INA219 Setup completed."));
#endif
}

int getMilliAmps()
{
	// Adafruit say the INA219 can be reset by sharp current loads so always recalbrate
	Wire.beginTransmission(hexAddress);
	Wire.write(INA219_REG_CALIBRATION);
	Wire.write((4096 >> 8) & 0xFF);
	Wire.write(4096 & 0xFF);
	Wire.endTransmission();

	// Initiate transmission to the device we want
	Wire.beginTransmission(hexAddress);

	// Tell device the register we want to write to (Current)
	Wire.write(INA219_REG_CURRENT);

	// Finish this "conversation"
	Wire.endTransmission();

	// Initiate transmission to the device we want
	Wire.beginTransmission(hexAddress);

	// Request the value (current in mA) two bytes
	Wire.requestFrom((int) hexAddress, 2);

	// Finish this conversation
	Wire.endTransmission();

	// delay required to allow INA219 to do the conversion (see samples)
	delayMicroseconds(4260);

	// Shift values to create properly formed integer
	// Note that no "conversation" start/end is required for a read
	// as the device is expecting this from the prep done above
	uint16_t value = ((Wire.read() << 8) | Wire.read());

	// Display the current being consumed
	// Current LSB = 100uA per bit (1000/100 = 10)
	int current = (value / 10) - 16; // LED takes 16mA
	current = current < 0 ? 0 : current;

	//Serial.print(F("Current (mA):"));
	//Serial.println(current);

	// if this is the FIRST charge pre-fill array to speed things up
	if (current == 0)
	{
		firstCharge = true;
	}

	if (firstCharge && current > 0)
	{
#ifdef DEBUGMSG
		Serial.print(F("First charge after zero, filling array with "));
		Serial.println(current);
#endif
		firstCharge = false;
		for (uint8_t cnt = 0; cnt < (sizeof(mA_Average) / 2); cnt++)
		{
			mA_Average[cnt] = current;
		}
	}

	// Send back averaged response
	uint8_t numberOfAverages = (sizeof(mA_Average) / 2) - 1;
	mA_Average[mAIdx] = current;
	mAIdx++;
	mAIdx = mAIdx > numberOfAverages ? 0 : mAIdx;

	uint16_t rollingAverage = 0;
	for (auto cnt = 0; cnt < numberOfAverages; cnt++)
	{
		rollingAverage += mA_Average[cnt];
	}

	return rollingAverage / numberOfAverages;
}

void displayBatteryPercent()
{
	// clear area of screen (leave heartbeat alone)
	display.fillRect(10, 0, display.width() - 10, display.height(), SSD1306_BLACK);

	// Write the message
	display.setTextSize(2);
	display.setTextColor(SSD1306_WHITE);
	display.setCursor(32, 1);   // x,y
	display.print("CHARGE");
	display.setCursor(50, 18);   // x,y
	display.print(batLevel);
	display.print("%");
	display.display();
}

void displayNotConnected() {
	display.clearDisplay();
	display.display();

	display.setTextSize(2);
	display.setTextColor(SSD1306_WHITE);
	display.setCursor(50, 1);   // x,y
	display.print("NOT");
	display.setCursor(10, 18);   // x,y
	display.print("CONNECTED");

	display.display();
}

void displayChargeStatus(bool charging)
		{
	// clear area of screen (leave heartbeat alone)
	display.fillRect(10, 0, display.width() - 10, display.height(), SSD1306_BLACK);

	// Write the message
	display.setTextSize(2);
	display.setTextColor(SSD1306_WHITE);
	if (charging)
	{
		int chargemA = getMilliAmps();
		display.setCursor(32, 1);   // x,y
		display.print("CHARGE");
		display.setCursor(40, 18);   // x,y
		display.print(chargemA);
		display.print("mA");
	} else
	{
		//display.setCursor(20, 16);   // x,y
		display.setCursor(32, 1);   // x,y
		display.print("PAUSED");
		display.setCursor(50, 18);   // x,y
		display.print(batLevel);
		display.print("%");
	}

	display.display();
}
