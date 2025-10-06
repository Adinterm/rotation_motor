# Rotation angle with sensor feedback

This Arduino project controls a motorized laser platform using an LDR sensor for calibration and (almost) precise angular rotation. It features:

- **Auto-calibration** using a light beam detected by an LDR.
- **Degree-based rotation** with accurate timing.
- **LCD I2C display** for real-time status.
- **Serial command interface** for control.

---

## Hardware Used

- Arduino UNO/Nano
- 16x2 I2C LCD (default address: `0x27`)
- LDR (Light Dependent Resistor)
- 10kΩ resistor (for voltage divider with LDR)
- Laser module
- A small mirror to reflect the light back.
- Dual motor driver (or digital output control/relay)
- DC motor (for rotation)

## Adjustment
- sensor value
- rotation "start" runs for 20s before switching off (default)

---

## Circuit Diagram (LDR to A0)
```plaintext
(VCC - 5V)
    |
    |
  [ LDR ]
    |
    |---------> A0 (to Arduino Analog Input)
    |
  [ 10kΩ Resistor ]
    |
    |
  (GND)

---

## 🖥️ Serial Commands

| Command      | Format        | Description                                     |
|--------------|---------------|-------------------------------------------------|
| `start,<dir>`| e.g. `start,1`| Begin auto-calibration. Dirct: 1=Left, 2=Right  |
| `cal`        | `cal`         | Show full rotation time (ms)                    |
| `deg,<angle>`| e.g. `deg,90` | Rotate by specific angle (1–360°)               |
| `stop`       | `stop`        | Stop current operation                          |

- All commands are **case-insensitive**.
- Responses are printed to **Serial Monitor**.

---

## LCD Display Info

The 16x2 LCD displays:
- Calibration progress and result
- Rotation angle during movement
- Stop or error messages

---

## Setup Instructions

1. Wire up components according to the diagram.
2. Upload the Arduino sketch (`.ino` file) to your board.
3. Open Serial Monitor (9600 baud).
4. Send commands like `start,1` and `deg,90`.
5. Watch rotation and LCD feedback in real-time.

---

## How It Works

- During calibration (`start,<dir>`), the laser beam is detected twice by the LDR.
- Time between detections is measured as one full rotation.
- Any degree rotation is calculated using:


---

## 🧪 Example Serial Session

```text
start,1
→ start,1        // After 2 light detections

cal
→ cal,1234       // Full rotation = 1234 ms

deg,90
→ deg,done       // Rotates 90°, then stops
