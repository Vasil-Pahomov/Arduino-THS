# General
This is Arduno-based air quality sensor and logger.
The following air quality parameters are measured:
- Temperature
- Humidity
- CO2 level
- total volatile organic compound (TVOC) level
- particle matter (dust) PM1.0, PM2.5 and PM10 levels

It can be powered either with batteries (I used 18560 recharge lithium) or with USB power supply.
The sensor stores data on MicroSD card and is able to download it to companion application with Bluetooth. An Android companion is designed: https://github.com/Vasil-Pahomov/AQS-Android or https://github.com/AlexeyKatsuro/ABTEx

# Circuit
![Circuit diagram](https://github.com/Vasil-Pahomov/Arduino-THS/raw/master/pics/sch.png)

# Part list

| Part | Image |
| ---- | ----- |
| Arduino Pro Mini (3.3V) | ![](https://github.com/Vasil-Pahomov/Arduino-THS/raw/master/pics/Arduino%20Pro%20Mini.jpg) |
| BME280 temperature and humidity sensor board | ![](https://github.com/Vasil-Pahomov/Arduino-THS/raw/master/pics/BME280.jpg) |
| MH-Z19B CO2 sensor | ![](https://github.com/Vasil-Pahomov/Arduino-THS/raw/master/pics/MH-Z19b.jpg) |
| CCS811 TVOC sensor board | ![](https://github.com/Vasil-Pahomov/Arduino-THS/raw/master/pics/CCS811.jpg) |
| PMS5003 dust sensor | ![](https://github.com/Vasil-Pahomov/Arduino-THS/raw/master/pics/PMS5003.jpg) |
| Nokia 5110 LCD | ![](https://github.com/Vasil-Pahomov/Arduino-THS/raw/master/pics/Nokia%205110%20display.jpg) |
| HC-06 bluetooth board | ![](https://github.com/Vasil-Pahomov/Arduino-THS/raw/master/pics/HC-06.jpg) |
| TP4056 charger board | ![](https://github.com/Vasil-Pahomov/Arduino-THS/raw/master/pics/TP4056%20board.jpg) |
| 5V boost converter | ![](https://github.com/Vasil-Pahomov/Arduino-THS/raw/master/pics/5V%20boost%20converter.jpg) |


# Log file format
Files are numbered starting from 0.log. Each file 10000 records. Record structure is defined in SDLog.h.
Once file numbers overcome 4000, it starts to delete files from the beginning.
Number of current file being written is stored in file named 'i' (2-byte unsigned word)

# Protocol format
## General
Organized as asyncronous messaging protocol. That is, both phone and sensor can send messages to each other. In fact, some messages resemble classis client-server communication.
## Message structure
| Field | Bytes | Description |
| ----- | ----- | ----------- |
| SGN | 2 | Signature=0xDEAF  |
| TYPE | 1 | Message type |
| DATA | ? | Message data, length depends on message type |

## Phone messages
### Sync time
TYPE=0x00
Sends current time to the sensor for sync up

| Field | Bytes | Description |
| ----- | ----- | ----------- |
| TIME | 4 | Unix timestamp |

### Read data
TYPE=0x01
Triggers sending data from the sensor

| Field | Bytes | Description |
| ----- | ----- | ----------- |
| FROMIDX | 4 | Index of starting record to read (including) |
| TOIDX | 4 | Index of ending record to read (including) |

### Reset storage
TYPE=0x02
Resets the storage (starts writing storage data anew)

The command has no body

### Calibrate CO2 sensor
TYPE=0x03
Sends calibration command to CO2 sensor. The sensor MUST be at least 20 minutes at open air environment (400 PPM).

The command has no body

## Sensor messages
### Status
TYPE=0x00

| Field | Bytes | Description |
| ----- | ----- | ----------- |
| STAT | 1 | Status:  0x00 if everything is OK, detailed see below |
| BAT | 1 | Battery percentage |
| LOGIDX | 4 | Number of last logged record (sequential, never overflows), returns 0xFFFFFFFF when there's SD card error |
| DATA | sizeof(Data) | Last sensor readings, Data structure (see Data.h) |

#### Status byte
Zero value means OK, one - failure

| Bit | Description |
| ----- | ----------- |
| 0 | BME sensor |
| 1 | MH-Z19 sensor |
| 2 | PMS5003 sensor |
| 3 | CCS811 sensor |
| 4 | SD card |
| 5 | Time was synchronized |
| 6 | RESERVED (always 0) |
| 7 | RESERVED (always 0) |

### Data
| Field | Bytes | Description |
| ----- | ----- | ----------- |
| FROMIDX | 4 | Index of starting record |
| TOIDX | 4 | Index of ending record (including) |
| DATA | sizeof(Data)*N | Data records, N=TOIDX-FROMIDX |
