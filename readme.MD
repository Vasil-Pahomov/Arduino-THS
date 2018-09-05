# General
This is Arduno-based air quality sensor and logger. More details will come later.

# Log file format
Files are numbered starting from 00000.log. Each file has a number of records with their ID, timestamp, and arbitrary amount of data blocks.
Timestamp is UNIX time. When the board starts anew and current time is unknown, the time start from the beginning of Unix epoch. After the time is known (when succesful phone sync has been done), the DT has correct value.
When the sync is done and the board doesn't know actual time, a special block TS is added.

| Field | Bytes | Description |
| ----- | ----- | ----------- |
| ID | 2 | ID of the record within this file, zero-based. So every file would have up to 65536 records |
| DT | 4 | Unix timestamp of the record date |
| BLK | 1 | Block type code |
| --- | - | Block data. Size depends on block type |
| BLK | 1 | Block type code |
| --- | - | Block data. Size depends on block type |
| ... |
| END | 1 | FF constant |

# Blocks

| Block | Code | Bytes | Description |
| --- | --- | --- | --- |
| TS | 0x80 | 4 | Time synchronization with the host. Data contains current datetime UNIX timestamp |
| TH | 0x01 | 4 | Temperature and humidity reading. Two 16-bit words; first holds temperature value in Celsius multiplied by 100, second holds humidity percent multiplied by 655.35 (so 0 means 0% RH and 65565 means 100% RH) |
| CO2 | 0x02 | 2 | CO2 reading, PPM |
| PM | 0x03 | 6 | Particle matter reading. Three 16-bit words, holding PM1.0, PM2.5 and PM10 values respectively |
| VOC | 0x04 | 2 | TVOC reading, in ppd |


