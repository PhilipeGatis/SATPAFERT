# Hardware Architecture Documentation: Aquarium Management System

## 📋 Overview

The system uses a **Defense in Depth** topology with noise filtering at the input (AC), stabilization on the main bus (DC), and inductive spike suppression at the edge (Actuators).

```
┌─────────────────────┐     ┌─────────────────────┐     ┌─────────────────────┐
│  LAYER 1: AC        │ ──► │  LAYER 2: DC        │ ──► │  LAYER 3: EDGE      │
│  Input/Protection   │     │  Bus/Logic          │     │  Peripherals/Suppr. │
│                     │     │                     │     │                     │
│  • AC Fuse          │     │  • 12.53V PSU       │     │  • Flyback Diodes   │
│  • NTC (Inrush)     │     │  • LM2596 → 5.1V    │     │  • Decoupling Caps  │
│  • EMI Filter (Y)   │     │  • Cap Bank         │     │  • Sensors (GX12)   │
│  • Canister Relay   │     │  • 8-Ch MOSFET      │     │  • Pumps + Valve    │
└─────────────────────┘     └─────────────────────┘     └─────────────────────┘
```

---

## ⚡ Layer 1: AC Input & Protection (Infrastructure)

The power input architecture focuses on simplicity and maximum electrical safety, delegating heavy filtering (like EMI filter and NTC/Inrush limits) to the professional circuits already built into the Switching Power Supply.

### Components

| Component | Function |
|---|---|
| **AC Glass Fuse** (3A to 5A) | Mandatory item: Protection against hard short circuits and fire risks |
| **SSR Relay Module 1CH** | Independent control via ESP32 for the Canister filter (AC) |

### Wiring Diagram

```text
[ IEC C14 INLET ]
      │
      ├─── [ GROUND PIN (Green) ] ────────┬───────► [ PSU GROUND (G) Terminal ]
      │                                   │
      │                                   └───────► [ Canister Outlet GROUND ]
      │         
      ├─── [ NEUTRAL PIN (Blue) ] ────────┬───────► [ PSU NEUTRAL (N) Terminal ]
      │                                   │
      │                                   └───────► [ Canister Outlet NEUTRAL ]
      │
      └─── [ LIVE PIN (Brown) ] ─── [ FUSE ] ─────┐
                                                  │
             ┌────────────────────────────────────┘
             │                                  
             ├───────────────────► [ PSU LIVE (L) Terminal ]
             │
             └───► [ SSR Relay Screw 1 ] ── SWITCH ──► [ SSR Relay Screw 2 ] ──► [ Canister Outlet LIVE ]
```

---

## 🔋 Layer 2: DC Bus & Logic (Distribution)

Responsible for converting power to logic levels and maintaining ESP32 stability during heavy load switching.

### Components

| Component | Function |
|---|---|
| **180W Switching PSU** | Adjusted to 12.53V |
| **T5AL250V Fuse** | Physical firewall for 8 pumps and sensors |
| **LM2596** | Step-down adjusted to 5.1V (ESP32 power) |
| **1× 470µF 16V** | In parallel at 12V MOSFET input (Prevents voltage dips on pump startup) |
| **4× 1000µF 10V** | In parallel at 5V output (Acts as a "UPS" for ESP32 to withstand line fluctuations and spikes) |

### Wiring Diagram

```text
[ PSU 12.53V ]
   │              │
 (V+)           (V─) ──────────────────────────────┐ (STAR GND)
   │              │                                │
[T5A FUSE]        │                                │
   │              │                                │
   ├──────────────┼────────────────┐               │
   │              │                │               │
[LM2596 IN+]  [LM2596 IN─]    [MOSFET VIN]   [MOSFET GND]
   │              │                │               │
(Output 5.1V)     │         (1× 470µF 16V)         │
   │              │                │               │
(4× 1000µF 10V)   │                │               │
   │              │                │               │
[ESP32 VIN]   [ESP32 GND]          │               │
   │              │         [ 8-CH MOSFET ]        │
   │              └────────────────┴───────────────┘
```

> [!IMPORTANT]
> **Star GND**: All negative references must return directly to the PSU V− terminal to avoid ground loops.

---

## 🌊 Layer 3: Peripherals & Suppression (Edge)

Responsible for mitigating inductive noise (Flyback) and stabilizing sensor readings on long cables (1.2m).

### Components

| Component | Application | Qty |
|---|---|---|
| **FR154 Diodes** (Fast Recovery) | Refill and Drain Pumps | 2 |
| **FR154 Diodes** (Fast Recovery) | Peristaltic Pumps (4 Fert + 1 Prime) | 5 |
| **1N5822 Diode** (Schottky) | Solenoid Valve (Channel 8) | 1 |
| **10µF / 22µF 50V Capacitors** | Sensor decoupling at cable ends (GX12) | — |

> [!NOTE]
> **Total: 8 flyback diodes** — one per motor channel (7× FR154 + 1× 1N5822). All installed at the cable end, next to the motor.

### Wiring Diagram — Actuators

```
[ MOSFET CHANNEL ] ──────────── (1.2m Wire) ──────┬────────── (PUMP +)
                                                   │
                                             [ FLYBACK DIODE ]
                                             (Stripe on POS+)
                                                   │
[ BUS GND ] ─────────────────── (1.2m Wire) ───────┴────────── (PUMP ─)
```

> [!CAUTION]
> **Flyback Diodes**: Must be installed at the **cable end** (next to the motor) to prevent the 1.2m cable from radiating noise like an antenna.

### Wiring Diagram — Ultrasonic Sensor (JSN-SR04T)

The sensor operates at 5V, but the ESP32 supports a maximum of **3.3V** on GPIOs. The **ECHO** pin requires a resistive voltage divider to reduce 5V → 3.3V before entering GPIO34.

| Component | Function |
|---|---|
| **1kΩ Resistor** (R1) | In series between ECHO and GPIO34 |
| **2kΩ Resistor** (R2) | Pull-down between GPIO34 and GND |

**Formula:** `Vout = 5V × R2 / (R1 + R2) = 5 × 2k / 3k = 3.33V` ✅

```
                           ┌──────────────────┐
  ESP32 GPIO18 (TRIG) ─────┤ TRIG    JSN-SR04T│
                           │                  │
  ESP32 5V ────────────────┤ VCC              │
                           │                  │
  GND ─────────────────────┤ GND              │
                           │                  │
                           │ ECHO ────┐       │
                           └──────────┼───────┘
                                      │
                                 [ R1 = 1kΩ ]
                                      │
                    ESP32 GPIO34 ──────┤
                                      │
                                 [ R2 = 2kΩ ]
                                      │
                                    (GND)
```

> [!WARNING]
> **Never connect ECHO directly to the ESP32** without the voltage divider. The 5V ECHO signal can permanently damage the ESP32 GPIO.

### Wiring Diagram — Capacitive Level Sensor (XKC-Y25-NPN)

Non-contact sensor that detects liquid presence through glass/tube walls. NPN open-collector output, directly compatible with ESP32's 3.3V.

| Sensor Pin | Connection |
|---|---|
| **VCC** (red) | ESP32 3.3V |
| **GND** (black) | GND |
| **OUT** (yellow) | GPIO 4 (`INPUT_PULLUP`) |

```
  ESP32 3.3V ─────────── VCC (red)
                                            ┌────────────────┐
  ESP32 GPIO4 ─────────── OUT (yellow) ────┤ XKC-Y25-NPN    │
   (INPUT_PULLUP)                          │ (glued on the  │
                                            │  outside of    │
  GND ─────────────────── GND (black)  ────┤  glass/tube)   │
                                            └────────────────┘
```

> [!TIP]
> **No external resistor needed.** The ESP32's internal pull-up (~45kΩ) is sufficient for the NPN open-collector. The sensor is glued to the **outside** of the aquarium glass at the desired maximum level.

### Wiring Diagram — TFT ST7735 Display (SPI)

1.8" color display, 128×160 pixels. Operates at **3.3V** — incompatible with 5V. Uses software SPI (bit-banging) on custom pins.

| Display Pin | ESP32 Connection | GPIO | Notes |
|---|---|---|---|
| **VCC** | 3.3V | — | ⚠️ Never use 5V |
| **GND** | GND | — | |
| **CS** | D15 | GPIO15 | Chip Select |
| **RESET** | EN pin | — | Hardware reset shared with ESP32 |
| **A0 (DC)** | TX2 | GPIO17 | Data/Command |
| **SDA (MOSI)** | D23 | GPIO23 | SPI Data |
| **SCK** | RX2 | GPIO16 | SPI Clock |
| **LED** | 3.3V | — | Fixed backlight (no free GPIO available) |

```
ESP32 3.3V  ────►  VCC
ESP32 GND   ────►  GND
ESP32 D15   ────►  CS
ESP32 EN    ────►  RESET
ESP32 TX2   ────►  A0 (DC)
ESP32 D23   ────►  SDA (MOSI)
ESP32 RX2   ────►  SCK
ESP32 3.3V  ────►  LED (fixed backlight)
```

> [!WARNING]
> The display **RESET** pin must connect to the ESP32 **EN** pin (not a GPIO). This ensures the display resets together with the microcontroller. With EN HIGH (normal operation), the display works normally.

> [!NOTE]
> On the ESP32 DevKit V1 breakout board, **D16** and **D17** are labeled **RX2** and **TX2** respectively. Use your terminal block labels to identify the correct positions.

---

### Wiring Diagram — Navigation Button

Physical button that controls the display: short press cycles pages; long press opens the menu (start TPA or toggle maintenance mode).

| Pin | Connection | GPIO | Configuration |
|---|---|---|---|
| **Terminal 1** | GPIO19 | GPIO19 | `INPUT_PULLUP` — active LOW |
| **Terminal 2** | GND | — | |

```
ESP32 GPIO19 ─────── [ BUTTON ] ──── GND
         INPUT_PULLUP, active LOW (pressed = LOW)
```

> [!TIP]
> The button can be mounted on the panel using a standard push-button or tactile switch. The firmware detects **short press** (page change) and **long press > 1s** (opens menu).

---

## 🛠️ Safe Implementation Notes

1. **AC Connections** — Insulate all 110V/220V solder joints and connections with heat-shrink tubing for maximum safety.

2. **Capacitor Polarity** — Check the negative stripe on all electrolytics (especially the 1000µF 10V operating at 5.1V).

3. **Flyback Diodes** — Must be installed at the cable end (next to the motor) to prevent the 1.2m cable from radiating noise like an antenna.

4. **Star GND** — All negative references must return directly to the PSU V− terminal to avoid ground loops.

5. **Siphon Effect (Hydraulics)** — To prevent continuous gravity draining of the aquarium after the drain pump is turned off, wire a solenoid valve in parallel with the drain pump (on the same MOSFET channel, ensuring a flyback diode for each) or make a breather hole in the hose.
