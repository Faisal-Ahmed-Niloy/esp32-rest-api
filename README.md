# Esp32-rest-api
ESP32 rest api (fastapi by python) with tft display

Only run the server.py .  The server-2.py is a test file.

🔄 Latest UI Updates (As per 9/11/25 - 16:12)
- ✅ Centered layout for work done and target count, split evenly across the screen
- ✅ Bold vertical divider visually separates the two halves, spanning both numbers and "pcs" labels
- ✅ Maximized number display
- ✅ "pcs" labels aligned directly below each number for clarity
- ✅ Progress bar logic capped at 100% to prevent overflow when done > target
- ✅ "Target Achieved!" message now centered and lowered for better visibility


<br>
<br>

Old Updates: (as per 8/11/25 - 17:19)


 Maintenance Mode:
- ✅ maintenance mode will be on after button-2 is pressed more then 2s and released.
- ✅ once triggered, maintenance screen stays on display until ESP32 restart.
- ✅ device locked: other buttons disables after maintenance triggered.

 Target Completion:
- ✅ shows congrats overlay in center for 3–4 seconds when target reached.
- ✅ added message “Target Achieved!” displayed below "Work Done:" line
- ✅ flag resets automatically when a new target is assigned.
