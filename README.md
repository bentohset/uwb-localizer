# UWB Positioning Localizer Proof of Concept
A coordinate tracking system with ESP32-UWB DW1000 modules
- 2x Anchors (ESP32-UWB)
- 1x Tag (ESP32-UWB)
- 1x BLE Scanner (ESP32)

Tag communicates with the Anchors via UWB to calculate its coordinate position. Once the tag moves into a designated location, advertises BLE information which the scanner picks up and retrieves the Tag's MAC address for authentication.