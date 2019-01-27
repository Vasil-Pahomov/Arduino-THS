# General
This is Arduno-based air quality sensor and logger. More details will come later.

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

## Sensor messages
### Status
TYPE=0x00

| Field | Bytes | Description |
| ----- | ----- | ----------- |
| STAT | 1 | Status:  0x00 if everything is OK, detailed see below |
| BAT | 1 | Battery percentage |
| LOGIDX | 4 | Number of last logged record (sequential, never overflows), returns 0xFFFFFFFF when there's SD card error |
| DATA | sizeof(Data) | Last sensor readings, Data structure |

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
