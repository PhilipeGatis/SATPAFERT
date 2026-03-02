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
│  • Filtro EMI (Y)   │     │  • Banco de Caps    │     │  • Sensores (RJ45)  │
│  • Relé Canister    │     │  • MOSFET 8 canais  │     │  • Bombas + Válvula │
└─────────────────────┘     └─────────────────────┘     └─────────────────────┘
```

---

## ⚡ Camada 1: Entrada e Proteção AC (Infraestrutura)

Responsável por filtrar interferências da rede elétrica e proteger a fonte contra surtos de inicialização (Inrush Current).

### Componentes

| Componente | Função |
|---|---|
| **Fusível AC** (3A a 5A) | Proteção contra curto-circuito bruto |
| **NTC 5D-11** | Limitador de corrente de partida (em série com a Fase) |
| **NDF 222M** (Capacitores Y) | Filtro EMI (Fase–Terra e Neutro–Terra) |
| **Módulo Relé SSR 1CH** | Controle independente para o Canister (AC) |

### Esquema de Ligação

```
[ TOMADA IEC C14 ]
      │
      ├─── [ PINO TERRA ] ────────────────────────┬───► [ Borne G Fonte ]
      │         │                                 │
      │         ├───[ NDF 222M ]─── (Conecta no L)│
      │         └───[ NDF 222M ]─── (Conecta no N)│
      │                                           │
      ├─── [ PINO N (Neutro) ] ───────────────────┼───► [ Borne N Fonte ]
      │                                           └───► [ Relé COMUM ]
      │
      └─── [ PINO L (Fase) ] ─── [ FUSÍVEL AC ] ──┐
                                                   │
              ┌────────────────────────────────────┘
              │                                    │
        [ NTC 5D-11 ]                       [ RELÉ ENTRADA ]
        (Em série)                          (Pino Fase/NA)
              │                                    │
        [ Borne L Fonte ]                   [ TOMADA CANISTER ]
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
| **2× 470µF 16V** | Em paralelo na entrada 12V do MOSFET |
| **4× 1000µF 10V** | Em paralelo na saída 5V (perto do ESP32) |

### Esquema de Ligação

```
[ FONTE 12.53V ]
   │              │
 (V+)           (V─) ─────────────────────────┐ (GND ESTRELA)
   │              │                            │
[FUSÍVEL T5A]     │                            │
   │              │                            │
   ├──────────────┼────────────┐               │
   │              │            │               │
[LM2596 IN+]  [LM2596 IN─]  [MOSFET VIN]  [MOSFET GND]
   │              │            │               │
(Saída 5.1V)      │      (2× 470µF 16V)        │
   │              │            │               │
[ESP32 VIN]   [ESP32 GND]      │               │
   │              │      [ 8 CANAIS MOSFET ]   │
(4× 1000µF 10V)   │            │               │
   │              └────────────┴───────────────┘
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
| **Capacitores 10µF / 22µF 50V** | Desacoplamento na ponta dos sensores (RJ45) | — |

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

---

## 🛠️ Notas de Implementação Segura

1. **Prioridade de Soldagem** — O NTC deve ser isolado com espaguete termo-retrátil de alta temperatura, pois aquece durante a operação.

2. **Polaridade dos Capacitores** — Verificar a listra negativa em todos os eletrolíticos (especialmente os de 1000µF 10V que estão operando em 5.1V).

3. **Diodos Flyback** — Devem ser instalados obrigatoriamente na ponta do fio (junto ao motor) para evitar que o cabo de 1,2m irradie ruído como uma antena.

4. **GND Estrela** — Todas as referências negativas devem retornar diretamente ao borne V− da fonte colmeia para evitar loops de terra.
