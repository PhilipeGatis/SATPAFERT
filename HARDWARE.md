# Documentação de Arquitetura de Hardware: Sistema de Gestão de Aquário

## 📋 Visão Geral

O sistema utiliza uma topologia de **Defesa em Profundidade**, com filtragem de ruído na entrada (AC), estabilização no barramento principal (DC) e supressão de picos indutivos na borda (Atuadores).

```
┌─────────────────────┐     ┌─────────────────────┐     ┌─────────────────────┐
│  CAMADA 1: AC       │ ──► │  CAMADA 2: DC       │ ──► │  CAMADA 3: BORDA    │
│  Entrada/Proteção   │     │  Barramento/Lógica  │     │  Periféricos/Supres.│
│                     │     │                     │     │                     │
│  • Fusível AC       │     │  • Fonte 12.53V     │     │  • Diodos Flyback   │
│  • NTC (Inrush)     │     │  • LM2596 → 5.1V    │     │  • Caps desacopl.   │
│  • Filtro EMI (Y)   │     │  • Banco de Caps    │     │  • Sensores (GX12)  │
│  • Relé Canister    │     │  • MOSFET 8 canais  │     │  • Bombas + Válvula │
└─────────────────────┘     └─────────────────────┘     └─────────────────────┘
```

---

## ⚡ Camada 1: Entrada e Proteção AC (Infraestrutura)

A arquitetura da entrada de energia foca em simplicidade e máxima segurança elétrica, delegando as proteções e filtragens (como Filtro EMI e Termistor NTC/Inrush) para os circuitos profissionais já embutidos na Fonte Colmeia.

### Componentes

| Componente | Função |
|---|---|
| **Fusível de Vidro AC** (3A a 5A) | Item obrigatório: Proteção contra curto-circuito bruto e risco de incêndio |
| **Módulo Relé SSR 1CH** | Controle independente via ESP32 para o filtro Canister (AC) |

### Esquema de Ligação

```text
[ TOMADA IEC C14 ]
      │
      ├─── [ PINO TERRA (Verde) ] ────────┬─────────► [ Borne G (TERRA) da Fonte ]
      │                                   │
      │                                   └─────────► [ Pino TERRA - Tomada Canister ]
      │         
      ├─── [ PINO NEUTRO (Azul) ] ────────┬─────────► [ Borne N (NEUTRO) da Fonte ]
      │                                   │
      │                                   └─────────► [ Pino NEUTRO - Tomada Canister ]
      │
      └─── [ PINO FASE (Marrom) ] ─── [ FUSÍVEL ] ──┐
                                                    │
             ┌──────────────────────────────────────┘
             │                                  
             ├───────────────────► [ Borne L (FASE) da Fonte ]
             │
             └───► [ Parafuso 1 do Relé SSR ] ── INTERRUPTOR ──► [ Parafuso 2 do Relé SSR ] ──► [ Pino FASE - Tomada Canister ]
```

---

## 🔋 Camada 2: Barramento DC e Lógica (Distribuição)

Responsável por converter a potência para os níveis lógicos e manter a estabilidade do ESP32 durante o chaveamento das cargas pesadas.

### Componentes

| Componente | Função |
|---|---|
| **Fonte Colmeia 180W** | Ajustada para 12.53V |
| **Fusível T5AL250V** | Firewall físico para as 8 bombas e sensores |
| **LM2596** | Step-down ajustado para 5.1V (alimentação ESP32) |
| **1× 470µF 16V** | Em paralelo na entrada 12V do MOSFET (Evita queda de tensão quando as bombas ligam) |
| **4× 1000µF 10V** | Em paralelo na saída 5V (Atua como "Nobreak" para o ESP32 aguentar flutuações e picos da rede) |

### Esquema de Ligação

```text
[ FONTE 12.53V ]
   │              │
 (V+)           (V─) ──────────────────────────────┐ (GND ESTRELA)
   │              │                                │
[FUSÍVEL T5A]     │                                │
   │              │                                │
   ├──────────────┼────────────────┐               │
   │              │                │               │
[LM2596 IN+]  [LM2596 IN─]    [MOSFET VIN]   [MOSFET GND]
   │              │                │               │
(Saída 5.1V)      │         (1× 470µF 16V)         │
   │              │                │               │
(4× 1000µF 10V)   │                │               │
   │              │                │               │
[ESP32 VIN]   [ESP32 GND]          │               │
   │              │         [ 8 CANAIS MOSFET ]    │
   │              └────────────────┴───────────────┘
```

> [!IMPORTANT]
> **GND Estrela**: Todas as referências negativas devem retornar diretamente ao borne V− da fonte colmeia para evitar loops de terra.

---

## 🌊 Camada 3: Periféricos e Supressão (Borda)

Responsável por mitigar o ruído indutivo (Flyback) e estabilizar a leitura dos sensores em cabos longos (1,2m).

### Componentes

| Componente | Aplicação | Qtd |
|---|---|---|
| **Diodos FR154** (Fast Recovery) | Bombas de Recalque e Esgoto | 2 |
| **Diodos FR154** (Fast Recovery) | Bombas Peristálticas (4 Fert + 1 Prime) | 5 |
| **Diodo 1N5822** (Schottky) | Válvula Solenoide (Canal 8) | 1 |
| **Capacitores 10µF / 22µF 50V** | Desacoplamento na ponta dos sensores (GX12) | — |

> [!NOTE]
> **Total: 8 diodos flyback** — um para cada canal com motor (7× FR154 + 1× 1N5822). Todos instalados na ponta do fio, junto ao motor.

### Esquema de Ligação — Atuadores

```
[ CANAL MOSFET ] ──────────── (Fio 1.2m) ──────┬────────── (BOMBA +)
                                               │
                                         [ DIODO FLYBACK ]
                                         (Listra no POS+)
                                               │
[ GND BARRAMENTO ] ─────────── (Fio 1.2m) ─────┴────────── (BOMBA ─)
```

> [!CAUTION]
> **Diodos Flyback**: Devem ser instalados obrigatoriamente **na ponta do fio** (junto ao motor) para evitar que o cabo de 1,2m irradie ruído como uma antena.

### Esquema de Ligação — Sensor Ultrassônico (JSN-SR04T)

O sensor opera em 5V, mas o ESP32 suporta no máximo **3.3V** nos GPIOs. O pino **ECHO** precisa de um divisor de tensão resistivo para reduzir o sinal de 5V → 3.3V antes de entrar no GPIO19.

| Componente | Função |
|---|---|
| **Resistor 1kΩ** (R1) | Em série entre ECHO e GPIO19 |
| **Resistor 2kΩ** (R2) | Pull-down entre GPIO19 e GND |

**Fórmula:** `Vout = 5V × R2 / (R1 + R2) = 5 × 2k / 3k = 3.33V` ✅

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
                    ESP32 GPIO19 ──────┤
                                      │
                                 [ R2 = 2kΩ ]
                                      │
                                    (GND)
```

> [!WARNING]
> **Nunca conecte o ECHO diretamente ao ESP32** sem o divisor de tensão. Os 5V do ECHO podem danificar permanentemente o GPIO do ESP32.

### Esquema de Ligação — Sensor Capacitivo de Nível (XKC-Y25-NPN)

Sensor sem contato que detecta a presença de líquido através da parede do vidro/tubo. Saída NPN open-collector, compatível diretamente com 3.3V do ESP32.

| Pino Sensor | Conexão |
|---|---|
| **VCC** (vermelho) | 3.3V do ESP32 |
| **GND** (preto) | GND |
| **OUT** (amarelo) | GPIO 4 (`INPUT_PULLUP`) |

```
  ESP32 3.3V ─────────── VCC (vermelho)
                                            ┌────────────────┐
  ESP32 GPIO4 ─────────── OUT (amarelo) ────┤ XKC-Y25-NPN    │
   (INPUT_PULLUP)                           │ (colado no     │
                                            │  vidro/tubo)   │
  GND ─────────────────── GND (preto)  ────┤                │
                                            └────────────────┘
```

> [!TIP]
> **Sem resistor externo necessário.** O pull-up interno do ESP32 (~45kΩ) é suficiente para o open-collector NPN. O sensor fica colado na **parte externa** do vidro do aquário no nível máximo desejado.

---

## 🛠️ Notas de Implementação Segura

1. **Conexões AC** — Isole todas as soldas e conexões de 110V/220V com espaguete termo-retrátil para segurança máxima.

2. **Polaridade dos Capacitores** — Verificar a listra negativa em todos os eletrolíticos (especialmente os de 1000µF 10V que estão operando em 5.1V).

3. **Diodos Flyback** — Devem ser instalados obrigatoriamente na ponta do fio (junto ao motor) para evitar que o cabo de 1,2m irradie ruído como uma antena.

4. **GND Estrela** — Todas as referências negativas devem retornar diretamente ao borne V− da fonte colmeia para evitar loops de terra.
