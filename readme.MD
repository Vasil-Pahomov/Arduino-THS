
# General
This is Arduno-based air quality sensor and logger. More details will come later.

# Log file format
Files are numbered starting from 0.log. Each file 10000 records. Record structure is defined in SDLog.h.
Once file numbers overcome 4000, it starts to delete files from the beginning.
Number of current file being written is stored in file named 'i' (2-byte unsigned word)

# Protocol format

## General
Organized as client-server, with phone is client and sensor is server. Communication is done in command-response manner.
## Command structure
| Field | Bytes | Description |
| ----- | ----- | ----------- |
| SGN | 2 | Signature=0xDEAF  |
| CMD | 1 | Command code |
| RTIME| 4 | Current time, UNIX timestamp |
| DATA | 8 | Command data |
| CHK | 1 | Checksum - sums command code,and all RTIME and all data bytes over 0xFF |

## Response structure
| Field | Bytes | Description |
| ----- | ----- | ----------- |
| SGN | 2 | Signature=0xDEAF  |
| CMD | 1 | Command code repeated |
| DATA | ? | Response data, length depends on command |
| CHK | - | Checksum - sums command code and all data bytes over 0xFF |
If command code isn't within accepted range, the response command code repeats incoming, and data is absent.
If checksum is wrong, the response command code is 0xFF and data is absent as well;


## Status command
CMD=0x00
Reads status and current sensor readings
#### Request data
No specific data, all zeros
#### Response data
| Field | Bytes | Description |
| ----- | ----- | ----------- |
| STAT | 1 | Status:  0x00 if everything is OK, detailed see below |
| BAT | 1 | Battery percentage |
| LOGIDX | 4 | Number of last logged record (sequential, never overflows), returns 0xFFFFFFFF when there's SD card error |
| DATA | 22 | Last sensor readings, DLog structure (are times redundant there?) |

#### Status byte
Zero value means OK, one - failure
| Bit | Description |
| ----- | ----------- |
| 0 | BME sensor |
| 1 | MH-Z19 sensor |
| 2 | PMS5003 sensor |
| 3 | CCS811 sensor |
| 4 | SD card |
| 5 | RESERVED (always 0) |
| 6 | RESERVED (always 0) |
| 7 | RESERVED (always 0) |

## Read command
 CMD=0x01
 Reads logged data
#### Request data
| Field | Bytes | Description |
| ----- | ----- | ----------- |
| FROMIDX | 4 | Index of starting record to read (including) |
| TOIDX | 4 | Index of ending record to read (including) |
#### Response data
| Field | Bytes | Description |
| ----- | ----- | ----------- |
| DATA | 22*N | DLog records (are times redundant there?), N=TOIDX-FROMIDX |

## Set display command
Sets display data and modes, backlight brightness? 
TBD
