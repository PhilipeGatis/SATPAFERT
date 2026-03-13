// ============================================================
// IARA - Caixa de Eletrônica (Versão Corte a Laser - DXF)
// Arquivo: electronics_enclosure_laser.scad
// Descrição: Painéis planificados 2D com finger-joints para
//            fabricação por corte a laser (acrílico/MDF 3mm)
// ============================================================
// Uso:
//   1. Abrir no OpenSCAD
//   2. Renderizar (F6)
//   3. Exportar: File > Export > Export as DXF
// ============================================================

$fn = 50;

// ============================================================
// PARÂMETROS GERAIS DA CAIXA
// ============================================================

box_width = 250; // mm (X)
box_depth = 220; // mm (Y)
box_height = 62; // mm (Z)
wall = 3; // mm - espessura das paredes
base_height = 62; // mm - altura da caixa
corner_r = 5; // mm - raio dos cantos arredondados

// -- Tampa de acrílico --
acrylic_thickness = 3; // mm
acrylic_tolerance = 0.5; // mm - folga para encaixe


// ============================================================
// PARÂMETROS DOS COMPONENTES INTERNOS
// ============================================================

psu_w = 165;
psu_d = 97.4;
psu_h = 42;

esp32_w = 63;  // MRD068A terminal adapter
esp32_d = 69;

mosfet_w = 99;
mosfet_d = 52;

lm2596_w = 65.7;
lm2596_d = 35.7;

ssr_w = 25.1;
ssr_d = 34.1;

rtc_w = 38;
rtc_d = 21.7;

tft_screen_w = 38.5;
tft_screen_h = 32;
tft_screen_offset_x = -2.25; // mm - tela deslocada do centro da placa (7.5mm / 12mm das bordas)
tft_board_w = 58;
tft_board_h = 34.4;
tft_mount_holes_spacing_w = 51.5;
tft_mount_holes_spacing_h = 28;
tft_mount_d = 2.2;

ultra_w = 41.5;
ultra_d = 28.5;

// ============================================================
// PARÂMETROS DOS CONECTORES
// ============================================================

canister_outlet_w = 40.5; // mm - largura do rasgo retangular
canister_outlet_h = 21.7; // mm - altura do rasgo retangular
pump_conn_d = 8;
pump_conn_qty = 8;

gx12_d = 12; // mm - diâmetro do furo de montagem GX12
gx12_qty = 3; // Ultrassônico, capacitivo, boia

// -- Botão de painel (push button momentâneo 12mm) --
btn_panel_d = 12; // mm - diâmetro do furo de montagem

// -- Ventilação --
vent_slot_w = 2;
vent_slot_l = 20;
vent_spacing = 4;
vent_qty = 8;

// -- Apoio da tampa (cantoneiras + ímãs) --
magnet_d = 6;            // mm - diâmetro do ímã neodímio
corner_support_size = 15; // mm - lado da peça de canto

// -- Legendas (gravação laser) --
label_size = 5; // mm - tamanho da fonte para legendas
label_font = "Liberation Sans:style=Bold";
label_color = "red"; // cor para visualização no OpenSCAD (não afeta DXF)

// ============================================================
// PARÂMETROS DE FINGER-JOINT
// ============================================================

mat_t = wall; // espessura do material (= 3mm)
fj_tooth = 10; // comprimento de cada dente
fj_kerf = 0.1; // compensação de kerf do laser (cada lado)

// ============================================================
// MÓDULOS UTILITÁRIOS
// ============================================================

// --- Gerador de borda com finger-joints (2D) ---
module fj_edge(edge_len, teeth = "tabs") {
  n_teeth = floor(edge_len / (fj_tooth * 2));
  actual_tooth = edge_len / (2 * n_teeth + 1);
  k = fj_kerf;

  if (teeth == "slots") {
    for (i = [0:n_teeth - 1]) {
      translate([actual_tooth + i * 2 * actual_tooth - k, -k])
        square([actual_tooth + 2 * k, mat_t + 2 * k]);
    }
  } else {
    for (i = [0:n_teeth - 1]) {
      translate([i * 2 * actual_tooth - k, -k])
        square([actual_tooth + 2 * k, mat_t + 2 * k]);
    }
    translate([n_teeth * 2 * actual_tooth - k, -k])
      square([actual_tooth + 2 * k, mat_t + 2 * k]);
  }
}

// --- Furos de standoff para módulos (2D) ---
module standoff_holes_2d(w, d, hole_d = 2.5) {
  inset = 2.5;
  if (w < 35) {
    positions = [
      [-w / 2 + inset, -d / 2 + inset],
      [w / 2 - inset, d / 2 - inset],
    ];
    for (pos = positions)
      translate([pos[0], pos[1]])
        circle(d=hole_d, $fn=20);
  } else {
    positions = [
      [-w / 2 + inset, -d / 2 + inset],
      [w / 2 - inset, -d / 2 + inset],
      [-w / 2 + inset, d / 2 - inset],
      [w / 2 - inset, d / 2 - inset],
    ];
    for (pos = positions)
      translate([pos[0], pos[1]])
        circle(d=hole_d, $fn=20);
  }
}

// --- Peça de canto para apoio da tampa (2D) ---
module corner_support_2d() {
  cs = corner_support_size;
  difference() {
    square([cs, cs], center=true);
    circle(d=magnet_d, $fn=30);
  }
  color(label_color)
    translate([0, magnet_d / 2 + 3])
      text("MAG", size=label_size - 2, halign="center", valign="center", font=label_font);
}

// Posições dos ímãs nos cantos (relativo ao centro da caixa)
corner_mag_x = box_width / 2 - mat_t - corner_support_size / 2;
corner_mag_y = box_depth / 2 - mat_t - corner_support_size / 2;

// Posição Y do rasgo de encaixe nos painéis (centro do rasgo)
corner_slot_y = (base_height - mat_t) / 2 - acrylic_thickness - mat_t / 2;


// ============================================================
// PAINEL DO FUNDO (base_2d_bottom)
// ============================================================
module base_2d_bottom() {
  difference() {
    square([box_width, box_depth], center=true);

    // --- Finger-joint slots nas 4 bordas ---
    translate([-box_width / 2, box_depth / 2 - mat_t])
      fj_edge(box_width, "slots");
    translate([-box_width / 2, -box_depth / 2])
      fj_edge(box_width, "slots");
    translate([-box_width / 2 + mat_t, -box_depth / 2])
      rotate([0, 0, 90])
        fj_edge(box_depth, "slots");
    translate([box_width / 2, -box_depth / 2])
      rotate([0, 0, 90])
        fj_edge(box_depth, "slots");

    // --- Furos de montagem dos módulos (espaçadores M2.5) ---
    translate([-70, 48]) {
      // MOSFET: furos com espaçamento 90mm (maior) × 43mm (menor)
      for (sx = [-1, 1])
        for (sy = [-1, 1])
          translate([sx * 43 / 2, sy * 90 / 2])
            circle(d=2.5, $fn=20);
    }
    translate([40, 65]) {
      // ESP32 MRD068A: furos com espaçamento 58mm × 64.5mm
      for (sx = [-1, 1])
        for (sy = [-1, 1])
          translate([sx * 58 / 2, sy * 64.5 / 2])
            circle(d=2.5, $fn=20);
    }
    translate([-20, 33]) {
      // LM2596: furos com espaçamento 60mm × 30.7mm
      for (sx = [-1, 1])
        for (sy = [-1, 1])
          translate([sx * 30.7 / 2, sy * 60 / 2])
            circle(d=2.5, $fn=20);
    }
    translate([90, 65]) {
      // ULTRA: furos 36mm × 23.5mm, girado 90° (23.5mm em X, 36mm em Y)
      for (sx = [-1, 1])
        for (sy = [-1, 1])
          translate([sx * 23.5 / 2, sy * 36 / 2])
            circle(d=2.5, $fn=20);
    }
    translate([92, 28]) {
      // SSR: furos com espaçamento 29mm × 19.5mm
      for (sx = [-1, 1])
        for (sy = [-1, 1])
          translate([sx * 29 / 2, sy * 19.5 / 2])
            circle(d=2.5, $fn=20);
    }
    translate([40, 8]) {
      // RTC DS3231: 3 furos (sem o canto superior-esquerdo, onde fica o conector)
      inset = 2.5;
      translate([-rtc_w / 2 + inset, -rtc_d / 2 + inset]) circle(d=2.5, $fn=20); // inf-esq
      translate([rtc_w / 2 - inset, -rtc_d / 2 + inset]) circle(d=2.5, $fn=20);  // inf-dir
      translate([rtc_w / 2 - inset, rtc_d / 2 - inset]) circle(d=2.5, $fn=20);   // sup-dir
    }

    // --- Ventilação no fundo (sob a fonte) ---
    for (i = [0:7]) {
      tw = 8 * vent_slot_w + 7 * vent_spacing;
      translate(
        [
          -tw / 2 + i * (vent_slot_w + vent_spacing),
          -box_depth / 2 + wall + 2 + psu_d / 2 - vent_slot_l / 2,
        ]
      )
        square([vent_slot_w, vent_slot_l]);
    }

    // --- Furos abas de montagem na parede ---
    translate([-box_width / 2 - 15 / 2, 0])
      circle(d=4.5, $fn=20);
    translate([box_width / 2 + 15 / 2, 0])
      circle(d=4.5, $fn=20);
  }

  // --- Abas de montagem (integradas ao fundo) ---
  mount_tab_w = 20;
  mount_tab_h = 15;
  difference() {
    translate([-box_width / 2 - mount_tab_h, -mount_tab_w / 2])
      square([mount_tab_h, mount_tab_w]);
    translate([-box_width / 2 - mount_tab_h / 2, 0])
      circle(d=4.5, $fn=20);
  }
  difference() {
    translate([box_width / 2, -mount_tab_w / 2])
      square([mount_tab_h, mount_tab_w]);
    translate([box_width / 2 + mount_tab_h / 2, 0])
      circle(d=4.5, $fn=20);
  }

  // --- Legendas dos módulos (gravação) ---
  color(label_color) {
    translate([0, -box_depth / 2 + wall + 2 + psu_d / 2 + vent_slot_l / 2 + 5])
      text("PSU", size=label_size, halign="center", valign="center", font=label_font);
    translate([-70, 48])
      text("MOSFET", size=label_size, halign="center", valign="center", font=label_font);
    translate([40, 65])
      text("ESP32", size=label_size, halign="center", valign="center", font=label_font);
    translate([-20, 33])
      text("LM2596", size=label_size, halign="center", valign="center", font=label_font);
    translate([90, 65])
      text("ULTRA", size=label_size, halign="center", valign="center", font=label_font);
    translate([92, 28])
      text("SSR", size=label_size, halign="center", valign="center", font=label_font);
    translate([40, 8])
      text("RTC", size=label_size, halign="center", valign="center", font=label_font);
    // Barramento de capacitores (em cima do LM2596)
    translate([-20, 68])
      text("4x CAP", size=label_size - 1, halign="center", valign="center", font=label_font);
  }
}

// ============================================================
// PAINEL FRONTAL (base_2d_front) — Sensores
// ============================================================
module base_2d_front() {
  panel_w = box_width;
  panel_h = base_height - mat_t;

  gx12_spacing = gx12_d + 10;
  sensor_z_2d = psu_h - 4;
  sensor_labels = ["ULTRA", "CAP", "BOIA"];

  difference() {
    square([panel_w, panel_h], center=true);

    // --- Finger-joints bordas laterais ---
    translate([-panel_w / 2 + mat_t, -panel_h / 2])
      rotate([0, 0, 90])
        fj_edge(panel_h, "tabs");
    translate([panel_w / 2, -panel_h / 2])
      rotate([0, 0, 90])
        fj_edge(panel_h, "tabs");

    // --- Finger-joints borda inferior ---
    translate([-panel_w / 2, -panel_h / 2])
      fj_edge(panel_w, "tabs");

    // --- Recortes sensores GX12 (furos circulares 12mm) ---
    for (i = [0:gx12_qty - 1]) {
      x_offset = -(gx12_qty - 1) * gx12_spacing / 2 + i * gx12_spacing;
      translate([x_offset, sensor_z_2d - panel_h / 2])
        circle(d=gx12_d, $fn=40);
    }

    // --- Rasgos para cantoneiras de apoio ---
    for (sx = [-1, 1])
      translate([sx * (panel_w / 2 - mat_t) - corner_support_size / 2, corner_slot_y - mat_t / 2])
        square([corner_support_size, mat_t]);
  }

  // --- Legendas dos sensores (gravação) ---
  color(label_color)
    for (i = [0:gx12_qty - 1]) {
      x_offset = -(gx12_qty - 1) * gx12_spacing / 2 + i * gx12_spacing;
      translate([x_offset, sensor_z_2d - panel_h / 2 + gx12_d / 2 + 4])
        text(sensor_labels[i], size=label_size - 1, halign="center", valign="center", font=label_font);
    }
}

// ============================================================
// PAINEL TRASEIRO (base_2d_back) — Ventilação
// ============================================================
module base_2d_back() {
  panel_w = box_width;
  panel_h = base_height - mat_t;

  difference() {
    square([panel_w, panel_h], center=true);

    // --- Finger-joints bordas laterais ---
    translate([-panel_w / 2 + mat_t, -panel_h / 2])
      rotate([0, 0, 90])
        fj_edge(panel_h, "tabs");
    translate([panel_w / 2, -panel_h / 2])
      rotate([0, 0, 90])
        fj_edge(panel_h, "tabs");

    // --- Finger-joints borda inferior ---
    translate([-panel_w / 2, -panel_h / 2])
      fj_edge(panel_w, "tabs");

    // --- Slots de ventilação traseira ---
    for (row = [0:2]) {
      vent_z = 8 + row * 14;
      translate([-60, vent_z - panel_h / 2])
        square([120, 3]);
    }

    // --- Rasgos para cantoneiras de apoio ---
    for (sx = [-1, 1])
      translate([sx * (panel_w / 2 - mat_t) - corner_support_size / 2, corner_slot_y - mat_t / 2])
        square([corner_support_size, mat_t]);
  }
}

// ============================================================
// PAINEL ESQUERDO (base_2d_left) — Bombas DC
// ============================================================
module base_2d_left() {
  panel_w = box_depth;
  panel_h = base_height - mat_t;

  cols = 2;
  rows = 4;
  col_spacing = 25;
  row_spacing = pump_conn_d + 6;
  start_z = 10;

  difference() {
    square([panel_w, panel_h], center=true);

    // --- Finger-joints borda inferior (encaixa no fundo) ---
    translate([-panel_w / 2, -panel_h / 2])
      fj_edge(panel_w, "tabs");

    // --- Finger-joints bordas verticais (encaixam no frontal/traseiro) ---
    translate([-panel_w / 2 + mat_t, -panel_h / 2])
      rotate([0, 0, 90])
        fj_edge(panel_h, "slots");
    translate([panel_w / 2, -panel_h / 2])
      rotate([0, 0, 90])
        fj_edge(panel_h, "slots");

    // --- Furos 8x bombas P4 ---
    for (col = [0:cols - 1])
      for (row = [0:rows - 1]) {
        y_offset = -pump_conn_qty / rows * row_spacing / 2 + row * row_spacing + row_spacing / 2;
        z_offset = start_z + col * col_spacing;
        translate([y_offset, z_offset - panel_h / 2])
          circle(d=pump_conn_d, $fn=30);
      }

    // --- Rasgos para cantoneiras de apoio ---
    for (sx = [-1, 1])
      translate([sx * (panel_w / 2 - mat_t) - corner_support_size / 2, corner_slot_y - mat_t / 2])
        square([corner_support_size, mat_t]);
  }

  // --- Legendas dos canais (gravação) ---
  color(label_color)
    for (col = [0:cols - 1])
      for (row = [0:rows - 1]) {
        ch_num = col * rows + row + 1;
        y_offset = -pump_conn_qty / rows * row_spacing / 2 + row * row_spacing + row_spacing / 2;
        z_offset = start_z + col * col_spacing;
        translate([y_offset, z_offset - panel_h / 2 + pump_conn_d / 2 + 3])
          text(str("CH", ch_num), size=label_size - 1, halign="center", valign="center", font=label_font);
      }
}

// ============================================================
// PAINEL DIREITO (base_2d_right) — AC IN + CANISTER
// ============================================================
module base_2d_right() {
  panel_w = box_depth;
  panel_h = base_height - mat_t;

  ac_x = box_depth / 4 + 25;
  ac_z = base_height / 2 + 2 - mat_t;
  can_x = 25;
  can_z = base_height / 2 + 2 - mat_t;

  difference() {
    square([panel_w, panel_h], center=true);

    // --- Finger-joints borda inferior (encaixa no fundo) ---
    translate([-panel_w / 2, -panel_h / 2])
      fj_edge(panel_w, "tabs");

    // --- Finger-joints bordas verticais (encaixam no frontal/traseiro) ---
    translate([-panel_w / 2 + mat_t, -panel_h / 2])
      rotate([0, 0, 90])
        fj_edge(panel_h, "slots");
    translate([panel_w / 2, -panel_h / 2])
      rotate([0, 0, 90])
        fj_edge(panel_h, "slots");

    // --- Tomada AC AS-08A (31.2 x 27.2mm) ---
    translate([ac_x - 27.2 / 2, ac_z - panel_h / 2 - 31.2 / 2])
      square([27.2, 31.2]);

    // --- Tomada Canister Margirius snap-in (40.5 x 21.7mm) ---
    translate([can_x - canister_outlet_w / 2, can_z - panel_h / 2 - canister_outlet_h / 2])
      square([canister_outlet_w, canister_outlet_h]);

    // --- Rasgos para cantoneiras de apoio ---
    for (sx = [-1, 1])
      translate([sx * (panel_w / 2 - mat_t) - corner_support_size / 2, corner_slot_y - mat_t / 2])
        square([corner_support_size, mat_t]);
  }

  // --- Legendas dos conectores (gravação) ---
  color(label_color) {
    translate([ac_x, ac_z - panel_h / 2 + 31.2 / 2 + 5])
      text("AC IN", size=label_size - 1, halign="center", valign="center", font=label_font);
    translate([can_x, can_z - panel_h / 2 + canister_outlet_h / 2 + 5])
      text("CANISTER", size=label_size - 1, halign="center", valign="center", font=label_font);
  }
}

// ============================================================
// TAMPA 2D (lid_2d) — Template para corte laser
// ============================================================
module lid_2d() {
  tft_x_offset = 40;
  tft_y_offset = 45;
  btn_x = tft_x_offset + tft_screen_w / 2 + 15;
  btn_y = tft_y_offset;

  // Posições dos ímãs na tampa (correspondem às cantoneiras)
  lid_mag_x = corner_mag_x;
  lid_mag_y = corner_mag_y;

  difference() {
    // Contorno retangular (cantos retos)
    square([box_width - acrylic_tolerance, box_depth - acrylic_tolerance], center=true);

    // Abertura TFT
    translate([tft_x_offset + tft_screen_offset_x - tft_screen_w / 2, tft_y_offset - tft_screen_h / 2])
      square([tft_screen_w, tft_screen_h]);

    // Furos TFT M2
    for (dx = [-tft_mount_holes_spacing_w / 2, tft_mount_holes_spacing_w / 2])
      for (dy = [-tft_mount_holes_spacing_h / 2, tft_mount_holes_spacing_h / 2])
        translate([tft_x_offset + dx, tft_y_offset + dy])
          circle(d=tft_mount_d);

    // Furo botão de painel (12mm) — ao lado do TFT
    translate([btn_x, btn_y])
      circle(d=btn_panel_d, $fn=40);

    // Ventilação
    for (side = [1, -1]) {
      for (i = [0:vent_qty - 1]) {
        tw = vent_qty * vent_slot_w + (vent_qty - 1) * vent_spacing;
        translate(
          [
            side * box_width / 4 - tw / 2 + i * (vent_slot_w + vent_spacing),
            -box_depth / 4 + tft_board_h / 2 + 10 - vent_slot_l / 2,
          ]
        )
          square([vent_slot_w, vent_slot_l]);
      }
    }

    // --- Furos para ímãs (4×, alinhados com as tiras frontal/traseira) ---
    for (sx = [-1, 1])
      for (sy = [-1, 1])
        translate([sx * lid_mag_x, sy * lid_mag_y])
          circle(d=magnet_d, $fn=30);

    // --- Puxador (semicírculo na borda frontal) ---
    pull_r = 10;
    translate([0, -(box_depth - acrylic_tolerance) / 2])
      circle(r=pull_r, $fn=40);
  }

  // --- Legendas da tampa (gravação) ---
  color(label_color) {
    translate([tft_x_offset, tft_y_offset - tft_screen_h / 2 - 5])
      text("TFT", size=label_size - 1, halign="center", valign="center", font=label_font);
    translate([btn_x, btn_y - btn_panel_d / 2 - 5])
      text("BTN", size=label_size - 1, halign="center", valign="center", font=label_font);
    translate([-(box_width - acrylic_tolerance) / 2 + 20, (box_depth - acrylic_tolerance) / 2 - 12])
      text("IARA", size=label_size + 2, halign="center", valign="center", font=label_font);
    // Legendas dos ímãs
    for (sx = [-1, 1])
      for (sy = [-1, 1])
        translate([sx * lid_mag_x, sy * lid_mag_y + magnet_d / 2 + 3])
          text("MAG", size=label_size - 2, halign="center", valign="center", font=label_font);
  }
}

// ============================================================
// LAYOUT COMPACTO — BASE (5 painéis em linhas paralelas)
// ============================================================
// Peças organizadas para minimizar o bounding box da chapa.
// Layout:
//   Linha 1: Fundo (280 × 220mm com abas)
//   Linha 2: Frontal (250 × 59) + Esquerdo (220 × 59)
//   Linha 3: Traseiro (250 × 59) + Direito (220 × 59)
// ============================================================
module base_2d() {
  spacing = 8; // mm entre peças
  panel_h = base_height - mat_t; // 59mm — altura dos painéis laterais
  mount_tab_h = 15;
  bottom_total_w = box_width + 2 * mount_tab_h; // 280mm com abas

  // --- Linha 1: Fundo (centro) ---
  base_2d_bottom();

  // --- Linha 2: Frontal + Esquerdo (acima do fundo) ---
  row2_y = box_depth / 2 + panel_h / 2 + spacing;

  // Frontal (250mm) — alinhado à esquerda
  translate([-(box_depth + spacing) / 4, row2_y])
    base_2d_front();

  // Esquerdo (220mm) — ao lado do frontal
  translate([box_width / 2 + spacing / 2 + box_depth / 2 - (box_depth + spacing) / 4, row2_y])
    base_2d_left();

  // --- Linha 3: Traseiro + Direito (abaixo do fundo) ---
  row3_y = -box_depth / 2 - panel_h / 2 - spacing;

  // Traseiro (250mm) — alinhado à esquerda
  translate([-(box_depth + spacing) / 4, row3_y])
    base_2d_back();

  // Direito (220mm) — ao lado do traseiro
  translate([box_width / 2 + spacing / 2 + box_depth / 2 - (box_depth + spacing) / 4, row3_y])
    base_2d_right();
}

// ============================================================
// LAYOUT OTIMIZADO — TUDO (mínimo de material)
// ============================================================
// Duas colunas lado a lado para minimizar chapa.
// Layout:
//   Col 1: Fundo (250×220) → Frontal (250×59) → Traseiro (250×59)
//   Col 2: Tampa (~250×220) → Esquerdo (220×59) → Direito (220×59)
// ============================================================
module all_2d() {
  spacing = 8; // mm entre peças
  panel_h = base_height - mat_t; // 59mm
  mount_tab_h = 15;

  // --- Coluna 1 (esquerda): Fundo + painéis maiores (250mm) ---
  col1_x = 0;

  // Fundo (em cima)
  translate([col1_x, 0])
    base_2d_bottom();

  // Frontal (abaixo do fundo)
  translate([col1_x, -box_depth / 2 - spacing - panel_h / 2])
    base_2d_front();

  // Traseiro (abaixo do frontal)
  translate([col1_x, -box_depth / 2 - spacing - panel_h - spacing - panel_h / 2])
    base_2d_back();

  // --- Coluna 2 (direita): Tampa + painéis menores (220mm) ---
  col2_x = box_width / 2 + mount_tab_h + spacing + (box_width - acrylic_tolerance) / 2;

  // Tampa (em cima, alinhada com o fundo)
  translate([col2_x, 0])
    lid_2d();

  // Esquerdo (abaixo da tampa)
  translate([col2_x, -box_depth / 2 - spacing - panel_h / 2])
    base_2d_left();

  // Direito (abaixo do esquerdo)
  translate([col2_x, -box_depth / 2 - spacing - panel_h - spacing - panel_h / 2])
    base_2d_right();

  // --- Cantoneiras de apoio (4 peças, abaixo dos painéis) ---
  strip_row_y = -box_depth / 2 - spacing - panel_h - spacing - panel_h - spacing;
  cs_spacing = corner_support_size + spacing;

  for (i = [0:3])
    translate([col2_x - 1.5 * cs_spacing + i * cs_spacing, strip_row_y - corner_support_size / 2])
      corner_support_2d();
}

// ============================================================
// RENDERIZAR
// ============================================================
// Descomente a linha desejada e exporte como DXF/SVG:

// base_2d();         // Painéis da base (5 peças compactas)
// lid_2d();          // Somente a tampa
all_2d();          // Base + tampa + tiras de apoio
// base_2d_bottom();  // Somente o painel do fundo (250×220mm — cabe em A3)

// ============================================================
// NOTAS DE FABRICAÇÃO
// ============================================================
// Material: Acrílico 3mm ou MDF 3mm
// Kerf: ajustar fj_kerf conforme a máquina (~0.05 a 0.15mm)
// Montagem:
//   - Painéis laterais encaixam no fundo via finger-joints
//   - Cola: cloreto de metileno (acrílico) ou cola branca (MDF)
//   - Módulos: fixar com espaçadores M2.5 metálicos nos furos
//   - Tampa: apoiada nas cantoneiras de canto, fixada com 4× ímãs neodímio 6mm
//   - Cantoneiras: colar nos 4 cantos internos da caixa (topo - 3mm)
// Legendas:
//   - No DXF, as legendas (text) aparecem como geometria separada
//   - No software da cortadora, selecione as legendas e configure
//     como "engrave" (gravação) ao invés de "cut" (corte)
//   - Potência sugerida: ~15-20% para acrílico, ~10-15% para MDF
// ============================================================
