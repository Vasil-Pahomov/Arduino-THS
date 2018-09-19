# General
This is Arduno-based air quality sensor and logger. More details will come later.

# Log file format
Files are numbered starting from 0.log. Each file has up to 65536 records. Record structure is defined in SDLog.h.
Once file numbers overcome 65000, it starts to roll logs over (overwrite old files starting from 0 again)
Number of current file being written is stored in file named 'i' (2-byte unsigned word)