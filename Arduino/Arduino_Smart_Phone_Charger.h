// Only modify this file to include
// - function definitions (prototypes)
// - include files
// - extern variable definitions
// In the appropriate section

#ifndef _Arduino_Intelligent_Phone_Charger_HC06_H_
#define _Arduino_Intelligent_Phone_Charger_HC06_H_
#include "Arduino.h"

//add your includes for the project Arduino_Intelligent_Phone_Charger_HC06 here


// INA219 Registers used in this sketch
#define INA219_REG_CALIBRATION (0x5)
#define INA219_REG_CONFIG (0x0)
#define INA219_REG_CURRENT (0x04)

// INA219 Config values used to measure current in mA
#define INA219_CONFIG_BVOLTAGERANGE_16V 		(0x0000)// 0-16V Range
#define INA219_CONFIG_BVOLTAGERANGE_32V 		(0x2000)// 0-32V Range
#define INA219_CONFIG_GAIN_8_320MV 				6144	// 8 x Gain
#define INA219_CONFIG_BADCRES_12BIT 			384 	// Bus ADC resolution bits
#define INA219_CONFIG_SADCRES_12BIT_1S_532US 	24 		// 1 x 12=bit sample
#define INA219_CONFIG_MODE_SANDBVOLT_CONTINUOUS 7		// Continuous conversion (not triggered)
#define INA219_CONFIG_SADCRES_12BIT_8S_4260US 	(0x0058)// 8 x 12-bit shunt samples averaged together


//end of add your includes here


//add your function definitions for the project Arduino_Intelligent_Phone_Charger_HC06 here
void displayNotConnected();



//Do not add code below this line
#endif /* _Arduino_Intelligent_Phone_Charger_HC06_H_ */
