// ============================================================
// IARA - Caixa de Eletrônica (Versão 3D - Impressão 3D)
// Arquivo: electronics_enclosure_3d.scad
// Descrição: Caixa paramétrica para impressão 3D dos módulos
//            eletrônicos do sistema IARA
// Baseado na Patola PB-710 (130 × 180 × 51mm)
// ============================================================

$fn = 50; // Aumentar para 120+ na exportação STL final

// ============================================================
// PARÂMETROS GERAIS DA CAIXA
// ============================================================

// -- Dimensões externas (baseado na Patola PB-710) --
box_width = 250; // mm (X)
box_depth = 220; // mm (Y) - para fonte 199×98mm + módulos
box_height = 62; // mm (Z) - altura da base (para fonte 42mm)
wall = 3; // mm - espessura das paredes
base_height = 62; // mm - altura da caixa
corner_r = 5; // mm - raio dos cantos arredondados

// -- Tampa de acrílico --
acrylic_thickness = 3; // mm - espessura do acrílico
acrylic_tolerance = 0.5; // mm - folga para encaixe

// -- Furos de fixação da tampa --
screw_d = 3.2; // mm - M3
screw_head_d = 6; // mm - cabeça do parafuso
screw_boss_d = 8; // mm - diâmetro do boss
screw_boss_h = base_height - wall; // mm

// ============================================================
// PARÂMETROS DOS COMPONENTES INTERNOS
// ============================================================

// -- Fonte Colmeia 180W (dimensões reais) --
// 199 x 98 x 42mm
psu_w = 165;
psu_d = 97.4;
psu_h = 42;

// -- ESP32 MRD068A (terminal adapter) --
// 63.5 x 69mm, furos: 58.5 x 64.5mm
esp32_w = 63.5;
esp32_d = 69;

// -- Módulo MOSFET 8 canais (dimensões reais) --
// PCB: 99 x 52mm, furos: 90 x 43mm, Ø3.2mm
mosfet_w = 99;
mosfet_d = 52;

// -- Módulo LM2596 (dimensões reais) --
// 65.7 x 35.7mm, furos: 60 x 30.7mm
lm2596_w = 65.7;
lm2596_d = 35.7;

// -- Módulo Relé SSR 1CH (dimensões reais) --
// 25mm(L) x 34mm(C) x 21mm(A)
ssr_w = 25.1;
ssr_d = 34.1;

// -- DS3231 RTC --
// 30.5 x 21.7mm, furos: 25.5 x 16.7mm, 3 furos
rtc_w = 30.5;
rtc_d = 21.7;

// -- TFT ST7735 1.8" 160x128 (dimensões reais) --
// PCB: 56 x 34 x 5mm
tft_screen_w = 38.5; // mm - área visível
tft_screen_h = 32;   // mm - área visível
tft_screen_offset_x = -2.25; // mm - tela deslocada do centro
tft_board_w = 58;  // mm - PCB largura
tft_board_h = 34.4; // mm - PCB altura
tft_mount_holes_spacing_w = 51.5; // mm
tft_mount_holes_spacing_h = 28; // mm
tft_mount_d = 2.2; // mm - M2

// -- Módulo Sensor Ultrassônico --
// 41 x 28.5mm
ultra_w = 41.5;
ultra_d = 28.5;

// ============================================================
// PARÂMETROS DOS CONECTORES NO PAINEL
// ============================================================

// -- Entrada AC: Tomada AS-08 IEC C14 com fusível integrado --
iec_fuse_w = 48; // mm - largura do recorte
iec_fuse_h = 28; // mm - altura do recorte
iec_fuse_mount_w = 40; // mm - distância entre furos de fixação
iec_fuse_mount_d = 3.5; // mm - diâmetro dos furos

// -- Tomada AC Canister: Margirius snap-in (padrão brasileiro) --
canister_outlet_w = 40.5; // mm - largura do rasgo retangular
canister_outlet_h = 21.7; // mm - altura do rasgo retangular

// -- Conectores DC para bombas (8x) --
pump_conn_d = 8; // mm - diâmetro do furo de montagem (P4)
pump_conn_qty = 8;

// -- Conectores GX12 para sensores (aviation plug) --
gx12_d = 12; // mm - diâmetro do furo de montagem
gx12_qty = 3; // Ultrassônico, capacitivo, boia

// -- Passagem de cabo (PG7 gland) --
pg7_d = 12; // mm - diâmetro do furo

// -- Cooler 40mm (ventilação MOSFET) --
fan_size = 40; // mm - tamanho do cooler
fan_hole_spacing = 32; // mm - distância entre furos de fixação
fan_screw_d = 3.5; // mm - M3
fan_depth = 10; // mm - espessura do cooler

// -- Ventilação --
vent_slot_w = 2; // mm
vent_slot_l = 20; // mm
vent_spacing = 4; // mm
vent_qty = 8;

// ============================================================
// MÓDULOS
// ============================================================

// --- Forma arredondada da caixa ---
module rounded_box(w, d, h, r) {
  hull() {
    for (x = [-w / 2 + r, w / 2 - r])
      for (y = [-d / 2 + r, d / 2 - r])
        translate([x, y, 0])
          cylinder(r=r, h=h);
  }
}

// --- Boss para parafuso ---
module screw_boss(h, outer_d, inner_d) {
  difference() {
    cylinder(d=outer_d, h=h);
    translate([0, 0, -0.1])
      cylinder(d=inner_d, h=h + 0.2);
  }
}

// --- Standoff para montagem de módulo ---
module standoff(h = 6, outer_d = 4, inner_d = 2.2) {
  difference() {
    cylinder(d=outer_d, h=h);
    translate([0, 0, h - 4])
      cylinder(d=inner_d, h=4.1);
  }
}

// --- Grid de standoffs para um módulo ---
module module_standoffs(w, d, h = 6) {
  inset = 2.5;
  if (w < 35) {
    // Módulos pequenos: apenas 2 standoffs diagonais
    positions = [
      [-w / 2 + inset, -d / 2 + inset],
      [w / 2 - inset, d / 2 - inset],
    ];
    for (pos = positions)
      translate([pos[0], pos[1], 0])
        standoff(h);
  } else {
    // Módulos grandes: 4 standoffs nos cantos
    positions = [
      [-w / 2 + inset, -d / 2 + inset],
      [w / 2 - inset, -d / 2 + inset],
      [-w / 2 + inset, d / 2 - inset],
      [w / 2 - inset, d / 2 - inset],
    ];
    for (pos = positions)
      translate([pos[0], pos[1], 0])
        standoff(h);
  }
}

// --- Slots de ventilação ---
module vent_slots(qty, slot_w, slot_l, spacing) {
  total_w = qty * slot_w + (qty - 1) * spacing;
  for (i = [0:qty - 1]) {
    translate([-total_w / 2 + i * (slot_w + spacing), -slot_l / 2, -0.1])
      cube([slot_w, slot_l, wall + 0.2]);
  }
}

// ============================================================
// PAINEL LATERAL ESQUERDO - Conectores DC (Bombas)
// ============================================================
module left_panel_cutouts() {
  cols = 2;
  rows = 4;
  col_spacing = 25;
  row_spacing = pump_conn_d + 6;
  start_z = wall + 10;

  for (col = [0:cols - 1])
    for (row = [0:rows - 1]) {
      y_offset = -pump_conn_qty / rows * row_spacing / 2 + row * row_spacing + row_spacing / 2;
      z_offset = start_z + col * col_spacing;
      translate([-box_width / 2 - 0.1, y_offset, z_offset])
        rotate([0, 90, 0])
          cylinder(d=pump_conn_d, h=wall + 0.2);
    }
}

// --- Etiquetas das bombas ---
module pump_labels() {
  cols = 2;
  rows = 4;
  col_spacing = 25;
  row_spacing = pump_conn_d + 6;
  start_z = wall + 10;
  labels = ["SOL", "DREN", "PRIME", "RECALQ", "DOSE1", "DOSE2", "DOSE3", "DOSE4"];

  for (col = [0:cols - 1])
    for (row = [0:rows - 1]) {
      idx = col * rows + row;
      y_offset = -pump_conn_qty / rows * row_spacing / 2 + row * row_spacing + row_spacing / 2;
      z_offset = start_z + col * col_spacing;
      translate([-box_width / 2 - 0.1, y_offset, z_offset + pump_conn_d / 2 + 3])
        rotate([90, 0, -90])
          linear_extrude(0.6)
            text(
              labels[idx], size=2.5, halign="center", valign="center",
              font="Liberation Sans:style=Bold"
            );
    }
}

// ============================================================
// PAINEL TRASEIRO
// ============================================================
module back_panel_cutouts(){}
module ac_labels(){}

// ============================================================
// PAINEL FRONTAL - Sensores (GX12)
// ============================================================
module front_panel_cutouts() {
  panel_y = box_depth / 2;
  gx12_spacing = gx12_d + 10; // espaçamento entre centros
  sensor_z = wall + psu_h - 4;

  for (i = [0:gx12_qty - 1]) {
    x_offset = -(gx12_qty - 1) * gx12_spacing / 2 + i * gx12_spacing;
    translate([x_offset, panel_y + 0.1, sensor_z])
      rotate([90, 0, 0])
        cylinder(d=gx12_d, h=wall + 0.2, $fn=40);
  }
}

module front_panel_labels() {
  panel_y = box_depth / 2;
  gx12_spacing = gx12_d + 10;
  labels = ["ULTRA", "CAP", "BOIA"];
  sensor_z = wall + psu_h - 4;

  for (i = [0:gx12_qty - 1]) {
    x_offset = -(gx12_qty - 1) * gx12_spacing / 2 + i * gx12_spacing;
    translate([x_offset, panel_y + 0.1, sensor_z + gx12_d / 2 + 4])
      rotate([-90, 180, 0])
        linear_extrude(0.6)
          text(
            labels[i], size=3.5, halign="center", valign="center",
            font="Liberation Sans:style=Bold"
          );
  }
}

// ============================================================
// PAINEL DIREITO - AC IN + CANISTER
// ============================================================
module right_panel_cutouts() {
  panel_x = box_width / 2;

  translate([panel_x + 0.1, box_depth / 4 + 25, base_height / 2 + 2])
    rotate([0, -90, 0])
      translate([-31.2 / 2, -27.2 / 2, 0])
        cube([31.2, 27.2, wall + 0.2]);

  translate([panel_x + 0.1, 25, base_height / 2 + 2])
    rotate([0, -90, 0])
      translate([-canister_outlet_h / 2, -canister_outlet_w / 2, 0])
        cube([canister_outlet_h, canister_outlet_w, wall + 0.2]);
}

module sensor_labels() {
  panel_x = box_width / 2;

  translate([panel_x + 0.1, box_depth / 4 + 25, base_height / 2 + 21.7 / 2 + 11])
    rotate([90, 0, 90])
      linear_extrude(0.6)
        text(
          "AC IN", size=5, halign="center", valign="center",
          font="Liberation Sans:style=Bold"
        );

  translate([panel_x + 0.1, 25, base_height / 2 + canister_outlet_h / 2 + 6])
    rotate([90, 0, 90])
      linear_extrude(0.6)
        text(
          "CANISTER", size=4, halign="center", valign="center",
          font="Liberation Sans:style=Bold"
        );
}

// ============================================================
// TAMPA DE ACRÍLICO (chapa reta)
// ============================================================
module lid() {
  tft_x_offset = 40;
  tft_y_offset = 65;

  color("LightCyan", 0.4)
    translate([0, 0, base_height])
      difference() {
        rounded_box(
          box_width - acrylic_tolerance,
          box_depth - acrylic_tolerance,
          acrylic_thickness, corner_r
        );

        translate(
          [
            tft_x_offset + tft_screen_offset_x - tft_screen_w / 2,
            tft_y_offset - tft_screen_h / 2,
            -0.1,
          ]
        )
          cube([tft_screen_w, tft_screen_h, acrylic_thickness + 0.2]);

        for (dx = [-tft_mount_holes_spacing_w / 2, tft_mount_holes_spacing_w / 2])
          for (dy = [-tft_mount_holes_spacing_h / 2, tft_mount_holes_spacing_h / 2])
            translate([tft_x_offset + dx, tft_y_offset + dy, -0.1])
              cylinder(d=tft_mount_d, h=acrylic_thickness + 0.2);

        for (pos = screw_positions())
          translate([pos[0], pos[1], -0.1])
            cylinder(d=screw_d + 0.3, h=acrylic_thickness + 0.2);

        translate([box_width / 4, -box_depth / 4 + tft_board_h / 2 + 10, 0])
          vent_slots(vent_qty, vent_slot_w, vent_slot_l, vent_spacing);
        translate([-box_width / 4, -box_depth / 4 + tft_board_h / 2 + 10, 0])
          vent_slots(vent_qty, vent_slot_w, vent_slot_l, vent_spacing);

        translate([0, box_depth / 4, acrylic_thickness - 0.3])
          linear_extrude(0.5)
            text(
              "I A R A", size=12, halign="center", valign="center",
              font="Liberation Sans:style=Bold"
            );
      }
}

// ============================================================
// POSIÇÕES DOS PARAFUSOS
// ============================================================
function screw_positions() =
  [
    [-box_width / 2 + 10, -box_depth / 2 + 10],
    [box_width / 2 - 10, -box_depth / 2 + 10],
    [-box_width / 2 + 10, box_depth / 2 - 10],
    [box_width / 2 - 10, box_depth / 2 - 10],
  ];

// ============================================================
// BASE PRINCIPAL
// ============================================================
module base() {
  color("SteelBlue", 0.9)
    difference() {
      union() {
        difference() {
          rounded_box(box_width, box_depth, base_height, corner_r);
          translate([0, 0, wall])
            rounded_box(
              box_width - wall * 2, box_depth - wall * 2,
              base_height, corner_r - wall
            );
        }

        rounded_box(box_width, box_depth, wall, corner_r);

        mount_tab_w = 20;
        mount_tab_h = 15;
        translate([-box_width / 2 - mount_tab_h, -mount_tab_w / 2, 0])
          cube([mount_tab_h, mount_tab_w, wall]);
        translate([box_width / 2, -mount_tab_w / 2, 0])
          cube([mount_tab_h, mount_tab_w, wall]);

        for (pos = screw_positions())
          translate([pos[0], pos[1], 0])
            screw_boss(screw_boss_h, screw_boss_d, screw_d);

        color("DarkRed")
          translate([-70, 48, wall - 0.5])
            module_standoffs(mosfet_d, mosfet_w, h=6);

        color("DarkGreen")
          translate([40, 65, wall - 0.5])
            module_standoffs(esp32_w, esp32_d, h=6);

        color("DarkBlue")
          translate([-20, 33, wall - 0.5])
            module_standoffs(lm2596_d, lm2596_w, h=6);

        color("Teal")
          translate([90, 65, wall - 0.5])
            module_standoffs(ultra_d, ultra_w, h=6);

        color("Orange")
          translate([90, 17.55, wall - 0.5])
            module_standoffs(ssr_w, ssr_d, h=6);

        color("Purple")
          translate([40, 11.35, wall - 0.5])
            module_standoffs(rtc_w, rtc_d, h=6);
      }

      back_panel_cutouts();
      left_panel_cutouts();
      right_panel_cutouts();
      front_panel_cutouts();

      translate([0, -box_depth / 2 + wall + 2 + psu_d / 2, 0])
        vent_slots(8, vent_slot_w, vent_slot_l, vent_spacing);

      for (row = [0:2]) {
        vent_z = wall + 8 + row * 14;
        translate([-60, -box_depth / 2 - 0.1, vent_z])
          cube([120, wall + 0.2, 3]);
      }

      translate([-box_width / 2 - 15 / 2, 0, wall / 2])
        cylinder(d=4.5, h=wall + 0.2, center=true, $fn=20);
      translate([box_width / 2 + 15 / 2, 0, wall / 2])
        cylinder(d=4.5, h=wall + 0.2, center=true, $fn=20);

      for (x = [-box_width / 2 + 15, box_width / 2 - 15])
        for (y = [-box_depth / 2 + 15, box_depth / 2 - 15])
          translate([x, y, -0.1])
            cylinder(d=10, h=1.5);
    }

  pump_labels();
  ac_labels();
  sensor_labels();
  front_panel_labels();
  module_area_labels();
}

// --- Etiquetas de identificação das áreas dos módulos ---
module module_area_labels() {
  label_h = 0.6;

  color("White")
    translate([0, -box_depth / 2 + wall + 2 + psu_d / 2, wall])
      linear_extrude(label_h)
        text(
          "FONTE 180W", size=5, halign="center", valign="center",
          font="Liberation Sans:style=Bold"
        );

  color("White")
    translate([-70, 48, wall])
      linear_extrude(label_h)
        text(
          "MOSFET", size=4, halign="center", valign="center",
          font="Liberation Sans:style=Bold"
        );
  color("White")
    translate([-70, 40, wall])
      linear_extrude(label_h)
        text(
          "8CH", size=3.5, halign="center", valign="center",
          font="Liberation Sans:style=Bold"
        );

  color("White")
    translate([40, 65, wall])
      linear_extrude(label_h)
        text(
          "ESP32", size=4, halign="center", valign="center",
          font="Liberation Sans:style=Bold"
        );

  color("White")
    translate([40, 11.35, wall])
      linear_extrude(label_h)
        text(
          "RTC", size=3, halign="center", valign="center",
          font="Liberation Sans:style=Bold"
        );

  color("White")
    translate([-20, 33, wall])
      linear_extrude(label_h)
        text(
          "LM2596", size=3.5, halign="center", valign="center",
          font="Liberation Sans:style=Bold"
        );

  color("White")
    translate([90, 65, wall])
      linear_extrude(label_h)
        text(
          "ULTRA", size=3.5, halign="center", valign="center",
          font="Liberation Sans:style=Bold"
        );

  color("White")
    translate([90, 17.55, wall])
      linear_extrude(label_h)
        text(
          "SSR", size=4, halign="center", valign="center",
          font="Liberation Sans:style=Bold"
        );

  color("White")
    translate([40, 55, wall])
      linear_extrude(label_h)
        text(
          "TFT", size=3, halign="center", valign="center",
          font="Liberation Sans:style=Bold"
        );
}

// ============================================================
// MONTAGEM COMPLETA
// ============================================================
module full_enclosure() {
  base();
  translate([box_width + 20, 0, 0])
    lid();
  %ghost_components();
}

// --- Ghost dos componentes ---
module ghost_components() {
  color("Silver", 0.3)
    translate([-psu_w / 2, -box_depth / 2 + wall + 2, wall])
      cube([psu_w, psu_d, psu_h]);

  color("DarkRed", 0.3)
    translate([-70 - mosfet_d / 2, 48 - mosfet_w / 2, wall + 6])
      cube([mosfet_d, mosfet_w, 15]);

  color("DarkGreen", 0.3)
    translate([40 - esp32_w / 2, 65 - esp32_d / 2, wall + 6])
      cube([esp32_w, esp32_d, 8]);

  color("DarkBlue", 0.3)
    translate([-20 - lm2596_d / 2, 33 - lm2596_w / 2, wall + 6])
      cube([lm2596_d, lm2596_w, 10]);

  color("Teal", 0.3)
    translate([90 - ultra_d / 2, 65 - ultra_w / 2, wall + 6])
      cube([ultra_d, ultra_w, 8]);

  color("Orange", 0.3)
    translate([90 - ssr_w / 2, 17.55 - ssr_d / 2, wall + 6])
      cube([ssr_w, ssr_d, 12]);

  color("Purple", 0.3)
    translate([40 - rtc_w / 2, 11.35 - rtc_d / 2, wall + 6])
      cube([rtc_w, rtc_d, 5]);

  color("Cyan", 0.3)
    translate([40 - tft_board_w / 2, 65 - tft_board_h / 2, base_height - 5])
      cube([tft_board_w, tft_board_h, 3]);
}

// ============================================================
// PLACA DE TESTE DE FURAÇÃO (impressão rápida)
// ============================================================
module test_plate() {
  plate_h = 3;

  color("LightBlue", 0.5)
    translate([0, 0, plate_h / 2])
      cube([box_width - 2 * wall, box_depth - 2 * wall, plate_h], center=true);

  color("DarkRed")
    translate([-70, 48, plate_h - 0.5])
      module_standoffs(mosfet_d, mosfet_w, h=6);

  color("DarkGreen")
    translate([40, 65, plate_h - 0.5])
      module_standoffs(esp32_w, esp32_d, h=6);

  color("DarkBlue")
    translate([-20, 33, plate_h - 0.5])
      module_standoffs(lm2596_d, lm2596_w, h=6);

  color("Teal")
    translate([90, 65, plate_h - 0.5])
      module_standoffs(ultra_d, ultra_w, h=6);

  color("Orange")
    translate([90, 17.55, plate_h - 0.5])
      module_standoffs(ssr_w, ssr_d, h=6);

  color("Purple")
    translate([40, 11.35, plate_h - 0.5])
      module_standoffs(rtc_w, rtc_d, h=6);

  module_area_labels();
}

// ============================================================
// RENDERIZAR
// ============================================================
// Descomente a linha desejada:

full_enclosure(); // Caixa completa com tampa e ghosts
// base();         // Somente a base (para exportar STL)
// lid();          // Somente a tampa 3D
// test_plate();   // Placa de teste de furação

// ============================================================
// NOTAS DE FABRICAÇÃO
// ============================================================
// BASE (Impressão 3D):
//   Material: PETG (resistência térmica e à umidade)
//   Camada: 0.2mm | Preenchimento: 30-40%
//   Suportes: necessários para recortes laterais
//   Orientação: fundo para baixo
//   Tempo estimado: ~10-14 horas
//
// TAMPA (Corte laser):
//   Material: Acrílico transparente 3mm
//   Usar o arquivo electronics_enclosure_laser.scad
//   Fixação: 4× parafusos M3×8mm + porcas
// ============================================================
