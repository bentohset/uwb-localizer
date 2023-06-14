# UWB Positioning Localizer Proof of Concept
A coordinate tracking system with ESP32-UWB DW3000 modules
- 2x Anchors (ESP32-UWB)
- 1x Tag (ESP32-UWB)
- Python Server running pygame for visualisation

Tag communicates with the Anchors via UWB to calculate its coordinate position. 
Python server receives data from the 2 Anchors by UDP Datagram