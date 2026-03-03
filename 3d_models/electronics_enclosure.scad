// ============================================================
// IARA - Caixa de Eletrônica (Electronics Enclosure)
// Arquivo: electronics_enclosure.scad
// Descrição: Caixa paramétrica para montagem dos módulos
//            eletrônicos do sistema IARA
// Baseado na Patola PB-710 (130 × 180 × 51mm)
// ============================================================

$fn = 50; // Aumentar para 120+ na exportação STL final

// ============================================================
// PARÂMETROS GERAIS DA CAIXA
// ============================================================

// -- Dimensões externas (baseado na Patola PB-710) --
box_width = 250; // mm (X) - aumentado para MOSFET 99mm
box_depth = 150; // mm (Y)
box_height = 60; // mm (Z) - altura da base (aumentado para fonte 40mm)
wall = 3; // mm - espessura das paredes
base_height = 60; // mm - altura da caixa
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
// 129 x 99 x 40mm
psu_w = 129;
psu_d = 99;
psu_h = 40;

// -- ESP32 DevKit V1 (38 pinos) --
// ~51 x 28mm, furos nos cantos
esp32_w = 51;
esp32_d = 28;
esp32_mount_holes = [[3, 3], [48, 3], [3, 25], [48, 25]];

// -- Módulo MOSFET 8 canais (dimensões reais) --
// PCB: 99 x 52mm, furos: 90 x 43mm, Ø3.2mm
mosfet_w = 99;
mosfet_d = 52;

// -- Módulo LM2596 (dimensões reais) --
// 61 x 34 x 12mm
lm2596_w = 61;
lm2596_d = 34;

// -- Módulo Relé SSR 1CH (dimensões reais) --
// 25mm(L) x 34mm(C) x 21mm(A)
ssr_w = 25;
ssr_d = 34;

// -- DS3231 RTC --
// ~38 x 22mm
rtc_w = 38;
rtc_d = 22;

// -- OLED SSD1306 0.96" 128x64 (dimensões reais) --
// PCB: 27 x 30 x 5mm
oled_screen_w = 25.5; // mm - área visível
oled_screen_h = 14; // mm - área visível
oled_board_w = 27; // mm - PCB largura
oled_board_h = 30; // mm - PCB altura
oled_mount_holes_spacing_w = 23.5; // mm
oled_mount_holes_spacing_h = 23.5; // mm
oled_mount_d = 2.2; // mm - M2

// ============================================================
// PARÂMETROS DOS CONECTORES NO PAINEL
// ============================================================

// -- Entrada AC: Tomada AS-08 IEC C14 com fusível integrado --
// Modelo: AS-08 Tripolar Macho c/ Porta-Fusível (embutir painel)
// Corpo: 31 x 30 x 21.2mm | Recorte painel: ~48 x 28mm
iec_fuse_w = 48; // mm - largura do recorte
iec_fuse_h = 28; // mm - altura do recorte
iec_fuse_mount_w = 40; // mm - distância entre furos de fixação
iec_fuse_mount_d = 3.5; // mm - diâmetro dos furos

// -- Tomada AC Canister: Padrão brasileiro NBR 14136 --
// Tomada fêmea de painel embutida, recorte circular
nbr_outlet_d = 37; // mm - diâmetro do recorte circular
nbr_outlet_mount_spacing = 45; // mm - distância entre furos de fixação
nbr_outlet_mount_d = 3.5; // mm - diâmetro dos furos

// -- Conectores DC para bombas (8x) --
// Conector P4 fêmea de painel (DC barrel jack 5.5x2.1mm)
pump_conn_d = 8; // mm - diâmetro do furo de montagem (P4)
pump_conn_qty = 8;

// -- Conectores RJ45 para sensores --
rj45_w = 16; // mm - largura do recorte
rj45_h = 14; // mm - altura do recorte
rj45_qty = 3; // Ultrassônico, capacitivo, boia

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
module standoff(h = 8, outer_d = 5, inner_d = 2.5) {
  difference() {
    cylinder(d=outer_d, h=h);
    translate([0, 0, h - 4])
      cylinder(d=inner_d, h=4.1);
  }
}

// --- Grid de standoffs para um módulo ---
module module_standoffs(w, d, h = 8) {
  positions = [
    [-w / 2 + 3, -d / 2 + 3],
    [w / 2 - 3, -d / 2 + 3],
    [-w / 2 + 3, d / 2 - 3],
    [w / 2 - 3, d / 2 - 3],
  ];
  for (pos = positions)
    translate([pos[0], pos[1], wall])
      standoff(h);
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
  // 8 conectores para bombas, distribuídos verticalmente em 2 colunas
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
  labels = ["CH1", "CH2", "CH3", "CH4", "CH5", "CH6", "CH7", "CH8"];

  for (col = [0:cols - 1])
    for (row = [0:rows - 1]) {
      idx = col * rows + row;
      y_offset = -pump_conn_qty / rows * row_spacing / 2 + row * row_spacing + row_spacing / 2;
      z_offset = start_z + col * col_spacing;
      translate([-box_width / 2 - 0.1, y_offset + pump_conn_d / 2 + 3, z_offset])
        rotate([90, 0, -90])
          linear_extrude(0.6)
            text(
              labels[idx], size=4, halign="center", valign="center",
              font="Liberation Sans:style=Bold"
            );
    }
}

// ============================================================
// PAINEL TRASEIRO - AC (IEC C14 c/ fusível + Tomada NBR 14136)
// ============================================================
module back_panel_cutouts() {
  panel_y = -box_depth / 2;

  // IEC C14 com fusível - entrada de força (esquerda)
  translate([-box_width / 4, panel_y - 0.1, base_height / 2 + 2])
    rotate([-90, 0, 0]) {
      // Recorte retangular (maior para acomodar o fusível)
      translate([-iec_fuse_w / 2, -iec_fuse_h / 2, 0])
        cube([iec_fuse_w, iec_fuse_h, wall + 0.2]);
      // Furos de fixação
      translate([-iec_fuse_mount_w / 2, 0, 0])
        cylinder(d=iec_fuse_mount_d, h=wall + 0.2);
      translate([iec_fuse_mount_w / 2, 0, 0])
        cylinder(d=iec_fuse_mount_d, h=wall + 0.2);
    }

  // Tomada NBR 14136 para canister (direita) - recorte circular
  translate([box_width / 4, panel_y - 0.1, base_height / 2 + 2])
    rotate([-90, 0, 0]) {
      // Recorte circular principal
      cylinder(d=nbr_outlet_d, h=wall + 0.2);
      // Furos de fixação (acima e abaixo)
      translate([0, nbr_outlet_mount_spacing / 2, 0])
        cylinder(d=nbr_outlet_mount_d, h=wall + 0.2);
      translate([0, -nbr_outlet_mount_spacing / 2, 0])
        cylinder(d=nbr_outlet_mount_d, h=wall + 0.2);
    }
}

// --- Etiquetas AC ---
module ac_labels() {
  panel_y = -box_depth / 2;

  // Label "AC IN + FUSE"
  translate([-box_width / 4, panel_y - 0.1, base_height / 2 + iec_fuse_h / 2 + 6])
    rotate([90, 0, 0])
      linear_extrude(0.6)
        text(
          "AC IN", size=5, halign="center", valign="center",
          font="Liberation Sans:style=Bold"
        );

  // Label "CANISTER" com símbolo BR
  translate([box_width / 4, panel_y - 0.1, base_height / 2 + nbr_outlet_d / 2 + 6])
    rotate([90, 0, 0])
      linear_extrude(0.6)
        text(
          "CANISTER", size=4, halign="center", valign="center",
          font="Liberation Sans:style=Bold"
        );
}

// ============================================================
// PAINEL DIREITO - Sensores (RJ45) + PG7
// ============================================================
module right_panel_cutouts() {
  panel_x = box_width / 2;
  rj45_spacing = rj45_w + 8;

  // 3 conectores RJ45
  for (i = [0:rj45_qty - 1]) {
    y_offset = -(rj45_qty - 1) * rj45_spacing / 2 + i * rj45_spacing;
    translate([panel_x + 0.1, y_offset, base_height / 2 - rj45_h / 2 + 5])
      rotate([0, -90, 0])
        rotate([0, 0, 90])
          translate([-rj45_w / 2, -rj45_h / 2, 0])
            cube([rj45_w, rj45_h, wall + 0.2]);
  }

  // PG7 cable gland (para cabos extras)
  translate([panel_x + 0.1, box_depth / 2 - 20, base_height / 2 + 5])
    rotate([0, -90, 0])
      cylinder(d=pg7_d, h=wall + 0.2);
}

// --- Etiquetas sensores ---
module sensor_labels() {
  panel_x = box_width / 2;
  rj45_spacing = rj45_w + 8;
  labels = ["ULTRA", "CAP", "BOIA"];

  for (i = [0:rj45_qty - 1]) {
    y_offset = -(rj45_qty - 1) * rj45_spacing / 2 + i * rj45_spacing;
    translate([panel_x + 0.1, y_offset, base_height / 2 + rj45_h / 2 + 8])
      rotate([90, 0, 90])
        linear_extrude(0.6)
          text(
            labels[i], size=4, halign="center", valign="center",
            font="Liberation Sans:style=Bold"
          );
  }
}

// ============================================================
// TAMPA DE ACRÍLICO (chapa reta)
// ============================================================
module lid() {
  // Posição do OLED alinhada com o ESP32 (frente-direita)
  oled_x_offset = box_width / 4 - 5;
  oled_y_offset = box_depth / 4 - 10;

  color("LightCyan", 0.4)
    translate([0, 0, base_height])
      difference() {
        // Chapa plana de acrílico com cantos arredondados
        rounded_box(
          box_width - acrylic_tolerance,
          box_depth - acrylic_tolerance,
          acrylic_thickness, corner_r
        );

        // Abertura retangular para o OLED (acima do ESP32)
        translate(
          [
            oled_x_offset - oled_screen_w / 2,
            oled_y_offset - oled_screen_h / 2,
            -0.1,
          ]
        )
          cube([oled_screen_w, oled_screen_h, acrylic_thickness + 0.2]);

        // Furos de montagem do OLED (M2)
        for (dx = [-oled_mount_holes_spacing_w / 2, oled_mount_holes_spacing_w / 2])
          for (dy = [-oled_mount_holes_spacing_h / 2, oled_mount_holes_spacing_h / 2])
            translate([oled_x_offset + dx, oled_y_offset + dy, -0.1])
              cylinder(d=oled_mount_d, h=acrylic_thickness + 0.2);

        // Furos para parafusos M3 (fixação na base)
        for (pos = screw_positions())
          translate([pos[0], pos[1], -0.1])
            cylinder(d=screw_d + 0.3, h=acrylic_thickness + 0.2);

        // Slots de ventilação
        translate([box_width / 4, -box_depth / 4 + oled_board_h / 2 + 10, 0])
          vent_slots(vent_qty, vent_slot_w, vent_slot_l, vent_spacing);
        translate([-box_width / 4, -box_depth / 4 + oled_board_h / 2 + 10, 0])
          vent_slots(vent_qty, vent_slot_w, vent_slot_l, vent_spacing);

        // Gravação "IARA" (raster no acrílico)
        translate([0, box_depth / 4, acrylic_thickness - 0.3])
          linear_extrude(0.5)
            text(
              "I A R A", size=12, halign="center", valign="center",
              font="Liberation Sans:style=Bold"
            );
      }
}

// --- Template 2D para corte laser (exportar como DXF) ---
module lid_2d() {
  oled_x_offset = box_width / 4 - 5;
  oled_y_offset = box_depth / 4 - 10;
  difference() {
    // Contorno
    hull() {
      for (
        x = [
          -(box_width - acrylic_tolerance) / 2 + corner_r,
          (box_width - acrylic_tolerance) / 2 - corner_r,
        ]
      )
        for (
          y = [
            -(box_depth - acrylic_tolerance) / 2 + corner_r,
            (box_depth - acrylic_tolerance) / 2 - corner_r,
          ]
        )
          translate([x, y]) circle(r=corner_r);
    }
    // OLED
    translate([oled_x_offset - oled_screen_w / 2, oled_y_offset - oled_screen_h / 2])
      square([oled_screen_w, oled_screen_h]);
    // Furos OLED M2
    for (dx = [-oled_mount_holes_spacing_w / 2, oled_mount_holes_spacing_w / 2])
      for (dy = [-oled_mount_holes_spacing_h / 2, oled_mount_holes_spacing_h / 2])
        translate([oled_x_offset + dx, oled_y_offset + dy])
          circle(d=oled_mount_d);
    // Furos M3
    for (pos = screw_positions())
      translate([pos[0], pos[1]]) circle(d=screw_d + 0.3);
    // Ventilação
    for (side = [1, -1]) {
      for (i = [0:vent_qty - 1]) {
        tw = vent_qty * vent_slot_w + (vent_qty - 1) * vent_spacing;
        translate(
          [
            side * box_width / 4 - tw / 2 + i * (vent_slot_w + vent_spacing),
            -box_depth / 4 + oled_board_h / 2 + 10 - vent_slot_l / 2,
          ]
        )
          square([vent_slot_w, vent_slot_l]);
      }
    }
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
        // Paredes da base
        difference() {
          rounded_box(box_width, box_depth, base_height, corner_r);
          translate([0, 0, wall])
            rounded_box(
              box_width - wall * 2, box_depth - wall * 2,
              base_height, corner_r - wall
            );
        }

        // Fundo sólido
        rounded_box(box_width, box_depth, wall, corner_r);

        // Bosses para parafusos
        for (pos = screw_positions())
          translate([pos[0], pos[1], 0])
            screw_boss(screw_boss_h, screw_boss_d, screw_d);

        // Standoffs para MOSFET 8CH (frente-esquerda)
        translate([-box_width / 4 + 5, box_depth / 4 - 5, 0])
          module_standoffs(mosfet_w, mosfet_d, h=6);

        // Standoffs para ESP32 (frente-direita)
        translate([box_width / 4 - 5, box_depth / 4 - 10, 0])
          module_standoffs(esp32_w, esp32_d, h=6);

        // Standoffs para LM2596 (traseira-esquerda)
        translate([-box_width / 4 + 5, -box_depth / 4 + 5, 0])
          module_standoffs(lm2596_w, lm2596_d, h=6);

        // Standoffs para SSR (traseira-direita, perto do canister)
        translate([box_width / 4, -box_depth / 4 + 10, 0])
          module_standoffs(ssr_w, ssr_d, h=6);

        // RTC DS3231 monta empilhado sobre o ESP32 (sem standoffs separados)

        // Guia/suporte para a fonte (trilhos laterais)
        translate([-psu_w / 2, -box_depth / 2 + wall + 2, wall]) {
          cube([3, psu_d, psu_h / 2]); // trilho esquerdo
          translate([psu_w - 3, 0, 0])
            cube([3, psu_d, psu_h / 2]);
          // trilho direito
        }
      }

      // ------ Cortes nos painéis ------

      // Painel traseiro (AC)
      back_panel_cutouts();

      // Painel esquerdo (bombas DC)
      left_panel_cutouts();

      // Painel direito (sensores)
      right_panel_cutouts();

      // Ventilação no fundo
      translate([0, 0, 0])
        vent_slots(6, vent_slot_w, vent_slot_l * 1.5, vent_spacing);

      // Cooler 40mm no painel frontal (acima do MOSFET)
      fan_x = -box_width / 4 + 5; // alinhado com o MOSFET
      fan_z = wall + fan_size / 2 + 5;
      translate([fan_x, box_depth / 2 - wall - 0.1, fan_z])
        rotate([-90, 0, 0]) {
          // Abertura circular
          cylinder(d=fan_size - 4, h=wall + 0.2);
          // Grille radial
          for (a = [0:30:150])
            rotate([0, 0, a])
              translate([-fan_size / 2 + 2, -1, 0])
                cube([fan_size - 4, 2, wall + 0.2]);
          // Furos M3
          for (dx = [-fan_hole_spacing / 2, fan_hole_spacing / 2])
            for (dy = [-fan_hole_spacing / 2, fan_hole_spacing / 2])
              translate([dx, dy, 0])
                cylinder(d=fan_screw_d, h=wall + 0.2);
        }

      // Pés de borracha (recesso)
      for (x = [-box_width / 2 + 15, box_width / 2 - 15])
        for (y = [-box_depth / 2 + 15, box_depth / 2 - 15])
          translate([x, y, -0.1])
            cylinder(d=10, h=1.5);
    }

  // Etiquetas em relevo (bombas)
  pump_labels();

  // Etiquetas AC
  ac_labels();

  // Etiquetas sensores
  sensor_labels();

  // Etiquetas internas dos módulos
  module_area_labels();
}

// --- Etiquetas de identificação das áreas dos módulos ---
module module_area_labels() {
  label_h = 0.6; // altura do relevo

  // FONTE 180W (fundo, centro-traseiro)
  color("White")
    translate([0, -box_depth / 2 + wall + 2 + psu_d / 2, wall])
      linear_extrude(label_h)
        text(
          "FONTE 180W", size=5, halign="center", valign="center",
          font="Liberation Sans:style=Bold"
        );

  // MOSFET 8CH (frente-esquerda)
  color("White")
    translate([-box_width / 4 + 5, box_depth / 4 - 5, wall])
      linear_extrude(label_h)
        text(
          "MOSFET", size=4, halign="center", valign="center",
          font="Liberation Sans:style=Bold"
        );
  color("White")
    translate([-box_width / 4 + 5, box_depth / 4 - 13, wall])
      linear_extrude(label_h)
        text(
          "8CH", size=3.5, halign="center", valign="center",
          font="Liberation Sans:style=Bold"
        );

  // ESP32 (frente-direita)
  color("White")
    translate([box_width / 4 - 5, box_depth / 4 - 10, wall])
      linear_extrude(label_h)
        text(
          "ESP32", size=4, halign="center", valign="center",
          font="Liberation Sans:style=Bold"
        );

  // LM2596 (traseira-esquerda)
  color("White")
    translate([-box_width / 4 + 5, -box_depth / 4 + 5, wall])
      linear_extrude(label_h)
        text(
          "LM2596", size=3.5, halign="center", valign="center",
          font="Liberation Sans:style=Bold"
        );

  // SSR (traseira-direita, perto do canister)
  color("White")
    translate([box_width / 4, -box_depth / 4 + 10, wall])
      linear_extrude(label_h)
        text(
          "SSR", size=4, halign="center", valign="center",
          font="Liberation Sans:style=Bold"
        );

  // DS3231 RTC (empilhado no ESP32, frente-direita)
  color("White")
    translate([box_width / 4 - 5, box_depth / 4 - 20, wall])
      linear_extrude(label_h)
        text(
          "RTC", size=3, halign="center", valign="center",
          font="Liberation Sans:style=Bold"
        );

  // Marcador posição OLED (acima do ESP32)
  color("White")
    translate([box_width / 4 - 5, box_depth / 4 - 3, wall])
      linear_extrude(label_h)
        text(
          "OLED", size=3, halign="center", valign="center",
          font="Liberation Sans:style=Bold"
        );
}

// ============================================================
// MONTAGEM COMPLETA
// ============================================================
module full_enclosure() {
  base();
  lid();

  // Visualização ghost dos componentes (apenas referência)
  %ghost_components();
}

// --- Ghost dos componentes (transparente, para referência) ---
module ghost_components() {
  // Fonte
  color("Silver", 0.3)
    translate([-psu_w / 2, -box_depth / 2 + wall + 2, wall])
      cube([psu_w, psu_d, psu_h]);

  // MOSFET (frente-esquerda)
  color("DarkRed", 0.3)
    translate([-box_width / 4 + 5 - mosfet_w / 2, box_depth / 4 - 5 - mosfet_d / 2, wall + 6])
      cube([mosfet_w, mosfet_d, 15]);

  // ESP32 (frente-direita)
  color("DarkGreen", 0.3)
    translate([box_width / 4 - 5 - esp32_w / 2, box_depth / 4 - 10 - esp32_d / 2, wall + 6])
      cube([esp32_w, esp32_d, 8]);

  // LM2596 (traseira-esquerda)
  color("DarkBlue", 0.3)
    translate([-box_width / 4 + 5 - lm2596_w / 2, -box_depth / 4 + 5 - lm2596_d / 2, wall + 6])
      cube([lm2596_w, lm2596_d, 10]);

  // SSR (traseira-direita, perto do canister)
  color("Orange", 0.3)
    translate([box_width / 4 - ssr_w / 2, -box_depth / 4 + 10 - ssr_d / 2, wall + 6])
      cube([ssr_w, ssr_d, 12]);

  // DS3231 RTC (empilhado sobre o ESP32)
  color("Purple", 0.3)
    translate([box_width / 4 - 5 - rtc_w / 2, box_depth / 4 - 10 - rtc_d / 2, wall + 6 + 8])
      cube([rtc_w, rtc_d, 5]);

  // OLED (acima do ESP32, montado na tampa)
  color("Cyan", 0.3)
    translate([box_width / 4 - 5 - oled_board_w / 2, box_depth / 4 - 10 - oled_board_h / 2, base_height - 5])
      cube([oled_board_w, oled_board_h, 3]);
}

// --- Renderizar ---
full_enclosure();

// ============================================================
// VARIANTES PARA FABRICAÇÃO
// ============================================================

// --- Imprimir apenas a base (3D) ---
// base();

// --- Exportar template 2D da tampa para corte laser ---
// (Usar: File > Export > .dxf)
// lid_2d();

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
//   Usar o módulo lid_2d() e exportar como DXF
//   Fixação: 4× parafusos M3×8mm + porcas
// ============================================================
