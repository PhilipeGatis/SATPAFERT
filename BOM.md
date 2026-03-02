# BOM — Bill of Materials

Lista completa de componentes do sistema SATPAFERT.

---

## 🧠 Controlador

| # | Componente | Especificação | Qtd |
|---|---|---|---|
| 1 | ESP32 DevKit V1 | 38 pinos, WROOM-32 | 1 |

---

## ⚡ Camada 1 — Entrada AC e Proteção

| # | Componente | Especificação | Qtd |
|---|---|---|---|
| 2 | Conector IEC C14 | Macho painel com fusível | 1 |
| 3 | Fusível AC | 3A–5A, 250V | 1 |
| 4 | NTC 5D-11 | Limitador inrush, 5Ω a frio | 1 |
| 5 | Capacitor NDF 222M | Capacitor Y, Fase–Terra | 1 |
| 6 | Capacitor NDF 222M | Capacitor Y, Neutro–Terra | 1 |
| 7 | Módulo Relé SSR 1CH | SSR, controle do Canister (AC) | 1 |

---

## 🔋 Camada 2 — Barramento DC e Lógica

| # | Componente | Especificação | Qtd |
|---|---|---|---|
| 8 | Fonte Colmeia 180W | 12V (ajustada 12.53V) | 1 |
| 9 | Fusível T5AL250V | 5A, proteção DC | 1 |
| 10 | Módulo LM2596 | Step-down ajustável (→ 5.1V) | 1 |
| 11 | Módulo MOSFET 8 canais | IRF540N ou equivalente, 12V | 1 |
| 12 | Capacitor eletrolítico 470µF | 16V, filtro entrada MOSFET | 2 |
| 13 | Capacitor eletrolítico 1000µF | 10V, filtro saída 5V (ESP32) | 4 |

---

## 🌊 Camada 3 — Atuadores

| # | Componente | Especificação | Qtd |
|---|---|---|---|
| 14 | Bomba peristáltica 12V | Fertilizantes (CH1–CH4) | 4 |
| 15 | Bomba peristáltica 12V | Prime / desclorificante (CH5) | 1 |
| 16 | Bomba submersível 12V | Esgoto / drenagem (CH6) | 1 |
| 17 | Bomba submersível 12V | Recalque / refill (CH7) | 1 |
| 18 | Válvula solenoide 12V | Normalmente fechada (CH8) | 1 |
| 19 | Filtro canister | Controlado via relé AC | 1 |

---

## 📡 Sensores

| # | Componente | Especificação | Qtd |
|---|---|---|---|
| 20 | JSN-SR04T | Ultrassônico à prova d'água, 5V | 1 |
| 21 | Sensor óptico de nível | Tipo boia, digital (GPIO 4) | 1 |
| 22 | Boia float switch | Nível do reservatório (GPIO 35) | 1 |
| 23 | DS3231 | Módulo RTC I2C (SDA 21 / SCL 22) | 1 |
| 24 | Display OLED 0.96" | SSD1306, I2C, 128×64 | 1 |

---

## 🛡️ Proteção e Filtragem

| # | Componente | Especificação | Qtd |
|---|---|---|---|
| 25 | Diodo FR154 | Fast Recovery, flyback bombas | 7 |
| 26 | Diodo 1N5822 | Schottky, flyback solenoide | 1 |
| 27 | Resistor 1kΩ | ¼W, divisor tensão ECHO (R1) | 1 |
| 28 | Resistor 2kΩ | ¼W, divisor tensão ECHO (R2) | 1 |
| 29 | Capacitor 10µF / 22µF | 50V, desacoplamento sensores | 2–4 |

---

## 🔌 Conectores e Acessórios

| # | Componente | Especificação | Qtd |
|---|---|---|---|
| 30 | Conector RJ45 fêmea | Painel, para sensores remotos | 2–4 |
| 31 | Cabo patch RJ45 | Cat5e, 1.2m (sensores) | 2–4 |
| 32 | Borne KRE / Wago | Conexões de potência | ~20 |
| 33 | Espaguete termo-retrátil | Isolamento NTC e soldas | 1m |
| 34 | Fio 22 AWG | Sinal / sensores | ~5m |
| 35 | Fio 18 AWG | Potência / bombas | ~10m |

---

## 📊 Resumo de Quantidades

| Categoria | Itens |
|---|---|
| Capacitores eletrolíticos | 6–10 |
| Diodos (flyback) | 8 |
| Resistores | 2 |
| Motores / bombas | 7 |
| Sensores | 4 |
| Módulos (ESP32, LM2596, MOSFET, Relé, RTC, OLED) | 6 |
