# esp32-rest-api
ESP32 rest api (fastapi by python) with tft display

Only run the server.py .  The server-2.py is a test file.

New Updates: as per 8/11/25 - 17:19
- Maintenance Mode:
- maintenance mode will be on after button-2 is pressed more then 2s and released.
- once triggered, maintenance screen stays on display until ESP32 restart.
- device locked: other buttons disables after maintenance triggered.

- Target Completion:
- shows congrats overlay in center for 3–4 seconds when target reached.
- added message “Target Achieved!” displayed below "Work Done:" line
- flag resets automatically when a new target is assigned.
