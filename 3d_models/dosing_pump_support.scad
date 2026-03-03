// ============================================================
// IARA - Suporte para 4 Bombas Dosadoras + 4 Garrafas Seachem 500ml
// Arquivo: dosing_pump_support.scad
// Descrição: Layout vertical - bombas em cima, garrafas embaixo
//            Montagem na parede, com fiação e tampa de acrílico
// ============================================================

// ---- Resolução de renderização ----
$fn = 60; // Aumentar para 120+ na exportação final STL

// ============================================================
// PARÂMETROS AJUSTÁVEIS
// ============================================================

// -- Garrafa Seachem 500ml (dimensões reais) --
bottle_diameter = 67; // mm (65mm real + 2mm folga)
bottle_height = 185; // mm (18.5cm)
bottle_holder_h = 40; // mm - altura do anel de retenção
bottle_wall = 2.5; // mm - espessura da parede do holder

// -- Bomba dosadora peristáltica (dimensões reais) --
pump_base_d = 54; // mm (diâmetro da base / orelhas)
pump_body_d = 48; // mm (diâmetro do corpo principal)
pump_motor_d = 31.8; // mm (diâmetro do motor no topo)
pump_height = 46.3; // mm (altura total)
pump_mount_holes = 50; // mm (distância entre furos das orelhas)
pump_mount_hole_d = 4; // mm (diâmetro dos furos de montagem)
pump_clearance = 2; // mm (folga ao redor da bomba)

// -- Layout geral --
num_stations = 4; // Número de estações
station_spacing = 85; // mm - espaçamento entre centros
wall_thickness = 3; // mm - espessura das paredes
base_thickness = 4; // mm - espessura da base/fundo

// -- Prateleira superior (bombas + fiação) --
shelf_depth = 80; // mm - profundidade da prateleira
shelf_height = 55; // mm - altura (bomba + espaço fiação)

// -- Fiação elétrica (atrás das bombas na prateleira) --
wiring_zone_d = 25; // mm - profundidade da zona de fiação
cable_entry_d = 10; // mm - diâmetro furo entrada de cabos
terminal_block_w = 12; // mm - largura de um borne

// -- Tampa de acrílico (prateleira superior) --
acrylic_thickness = 3; // mm
lid_screw_d = 3.2; // mm - M3
lid_post_d = 7; // mm - diâmetro dos postes

// -- Montagem na parede --
mount_hole_d = 5; // mm
mount_slot_length = 8; // mm

// ============================================================
// DIMENSÕES CALCULADAS
// ============================================================

total_width = (num_stations - 1) * station_spacing + pump_base_d + pump_clearance * 2 + wall_thickness * 2 + 10;
shelf_inner_d = shelf_depth - wall_thickness * 2;

// ============================================================
// MÓDULOS UTILITÁRIOS
// ============================================================

module rounded_rect(w, d, h, r = 5) {
  hull() {
    for (x = [-w / 2 + r, w / 2 - r])
      for (y = [-d / 2 + r, d / 2 - r])
        translate([x, y, 0])
          cylinder(r=r, h=h);
  }
}

// ============================================================
// PRATELEIRA SUPERIOR (bombas + fiação)
// ============================================================

module pump_shelf() {
  difference() {
    union() {
      // Caixa da prateleira (bandeja com fundo e paredes)
      difference() {
        rounded_rect(total_width, shelf_depth, shelf_height, 4);
        translate([0, 0, base_thickness])
          rounded_rect(
            total_width - wall_thickness * 2,
            shelf_depth - wall_thickness * 2,
            shelf_height, 3
          );
      }
      // Fundo sólido
      rounded_rect(total_width, shelf_depth, base_thickness, 4);

      // Divisória entre zona de bombas (frente) e fiação (trás)
      translate(
        [
          -total_width / 2 + wall_thickness + 1,
          -shelf_depth / 2 + shelf_depth - wiring_zone_d - wall_thickness,
          base_thickness,
        ]
      )
        cube([total_width - wall_thickness * 2 - 2, wall_thickness, shelf_height * 0.7]);
    }

    // Furos para mangueiras passarem pelo fundo (para as garrafas embaixo)
    for (i = [0:num_stations - 1]) {
      x = -( (num_stations - 1) * station_spacing) / 2 + i * station_spacing;
      // Furo da mangueira de sucção (garrafa → bomba)
      translate([x - 10, -shelf_depth / 2 + shelf_depth / 2 - wiring_zone_d / 2 - 5, -0.1])
        cylinder(d=8, h=base_thickness + 0.2);
      // Furo da mangueira de saída (bomba → aquário)
      translate([x + 10, -shelf_depth / 2 + shelf_depth / 2 - wiring_zone_d / 2 - 5, -0.1])
        cylinder(d=8, h=base_thickness + 0.2);
    }

    // Passagens de fios na divisória (um por bomba)
    for (i = [0:num_stations - 1]) {
      x = -( (num_stations - 1) * station_spacing) / 2 + i * station_spacing;
      divider_y = -shelf_depth / 2 + shelf_depth - wiring_zone_d - wall_thickness;
      translate([x - 8, divider_y - 0.1, base_thickness + 5])
        cube([16, wall_thickness + 0.2, 12]);
    }

    // Furos de entrada de cabos (painel traseiro - vindos da caixa eletrônica)
    for (x_off = [-total_width / 4, total_width / 4]) {
      translate([x_off, shelf_depth / 2 - 0.1, shelf_height / 2 + 5])
        rotate([90, 0, 0])
          rotate([0, 0, 0])
            cylinder(d=cable_entry_d, h=wall_thickness + 0.2);
    }

    // Ventilação lateral
    for (side = [-1, 1]) {
      translate(
        [
          side * (total_width / 2 - wall_thickness - 0.1),
          shelf_depth / 2 - wiring_zone_d - 5,
          base_thickness + 8,
        ]
      )
        cube([wall_thickness + 0.2, wiring_zone_d - 5, 20]);
    }
  }
}

// --- Suporte individual da bomba (circular, dentro da prateleira) ---
module pump_bracket() {
  bracket_outer_d = pump_base_d + pump_clearance * 2 + wall_thickness * 2;
  cradle_h = pump_height * 0.6;

  difference() {
    union() {
      // Base circular
      cylinder(d=bracket_outer_d, h=base_thickness);

      // Berço cilíndrico
      difference() {
        cylinder(d=bracket_outer_d, h=cradle_h);
        translate([0, 0, base_thickness])
          cylinder(d=pump_base_d + pump_clearance * 2, h=cradle_h + 1);
      }

      // Clips de retenção (3 a 120°)
      for (a = [60, 180, 300]) {
        rotate([0, 0, a])
          translate([pump_body_d / 2 + pump_clearance, 0, cradle_h - 6]) {
            difference() {
              cylinder(d=7, h=6);
              translate([-5, -3.5, 4])
                cube([5, 7, 3]);
            }
          }
      }
    }

    // Furos de montagem (2 orelhas opostas)
    for (dx = [-pump_mount_holes / 2, pump_mount_holes / 2])
      translate([dx, 0, -0.1])
        cylinder(d=pump_mount_hole_d, h=base_thickness + 0.2);

    // Abertura frontal para mangueiras
    translate([-pump_body_d / 3, -bracket_outer_d / 2 - 1, base_thickness])
      cube([pump_body_d * 2 / 3, bracket_outer_d / 3, cradle_h]);

    // Abertura traseira para fios
    translate([-10, bracket_outer_d / 4, base_thickness + 5])
      cube([20, bracket_outer_d / 2, 15]);
  }
}

// --- Postes para bornes na zona de fiação ---
module wiring_mounts() {
  for (i = [0:num_stations - 1]) {
    x = -( (num_stations - 1) * station_spacing) / 2 + i * station_spacing;
    wiring_y = shelf_depth / 2 - wiring_zone_d / 2;

    // Posts para borne
    for (dx = [-terminal_block_w / 2 - 1, terminal_block_w / 2 + 1]) {
      translate([x + dx, wiring_y, base_thickness]) {
        difference() {
          cylinder(d=5, h=8);
          translate([0, 0, 3])
            cylinder(d=2.5, h=5.1);
        }
      }
    }

    // Etiqueta M1-M4
    translate([x, wiring_y + 12, base_thickness])
      linear_extrude(0.6)
        text(
          str("M", i + 1), size=5, halign="center", valign="center",
          font="Liberation Sans:style=Bold"
        );
  }
}

// --- Etiquetas CH1-CH4 na frente da prateleira ---
module pump_labels() {
  for (i = [0:num_stations - 1]) {
    x = -( (num_stations - 1) * station_spacing) / 2 + i * station_spacing;
    translate([x, -shelf_depth / 2 - 0.1, shelf_height / 2])
      rotate([90, 0, 0])
        linear_extrude(0.6)
          text(
            str("CH", i + 1), size=6, halign="center", valign="center",
            font="Liberation Sans:style=Bold"
          );
  }
}

// ============================================================
// PRATELEIRA INFERIOR (garrafas)
// ============================================================

// Distância vertical entre prateleira e porta-garrafas
shelf_to_bottles = 20; // mm - espaço para mangueiras

module bottle_shelf() {
  shelf_w = total_width;
  shelf_d = bottle_diameter + bottle_wall * 2 + 10;

  difference() {
    union() {
      // Base da prateleira de garrafas
      rounded_rect(shelf_w, shelf_d, base_thickness, 4);

      // Paredes laterais baixas (guarda)
      difference() {
        rounded_rect(shelf_w, shelf_d, bottle_holder_h, 4);
        translate([0, 0, base_thickness])
          rounded_rect(
            shelf_w - wall_thickness * 2,
            shelf_d - wall_thickness * 2,
            bottle_holder_h + 1, 3
          );
      }

      // Holders individuais das garrafas
      for (i = [0:num_stations - 1]) {
        x = -( (num_stations - 1) * station_spacing) / 2 + i * station_spacing;
        translate([x, 0, 0])
          bottle_holder();
      }

      // Divisórias entre garrafas
      for (i = [1:num_stations - 1]) {
        x = -( (num_stations - 1) * station_spacing) / 2 + (i - 0.5) * station_spacing;
        translate([x - 1.5, -shelf_d / 2 + wall_thickness, base_thickness])
          cube([3, shelf_d - wall_thickness * 2, bottle_holder_h * 0.6]);
      }
    }

    // Furos de montagem na parede
    bottle_mount_holes(shelf_w, shelf_d);
  }

  // Etiquetas das garrafas
  for (i = [0:num_stations - 1]) {
    x = -( (num_stations - 1) * station_spacing) / 2 + i * station_spacing;
    translate([x, -shelf_d / 2 - 0.1, bottle_holder_h / 2])
      rotate([90, 0, 0])
        linear_extrude(0.6)
          text(
            str("CH", i + 1), size=5, halign="center", valign="center",
            font="Liberation Sans:style=Bold"
          );
  }
}

// --- Holder individual da garrafa ---
module bottle_holder() {
  difference() {
    cylinder(d=bottle_diameter + bottle_wall * 2, h=bottle_holder_h);
    translate([0, 0, base_thickness])
      cylinder(d=bottle_diameter, h=bottle_holder_h);
    // Fenda frontal para encaixe/retirada
    translate([-bottle_diameter / 4, -bottle_diameter / 2 - bottle_wall - 1, base_thickness + 10])
      cube([bottle_diameter / 2, bottle_diameter + bottle_wall * 2 + 2, bottle_holder_h]);
  }

  // Abas de retenção (3 pontos)
  for (a = [0, 120, 240]) {
    rotate([0, 0, a])
      translate([bottle_diameter / 2 - 1, 0, bottle_holder_h - 4])
        difference() {
          cylinder(d=5, h=4);
          translate([0, 0, -0.1])
            cylinder(d=bottle_diameter - 2, h=4.2);
        }
  }

  // Dreno no fundo
  difference() {
    cylinder(d=bottle_diameter + bottle_wall * 2, h=base_thickness);
    translate([0, 0, -0.1])
      cylinder(d=18, h=base_thickness + 0.2);
  }
}

// ============================================================
// FUROS DE MONTAGEM NA PAREDE
// ============================================================

module shelf_mount_holes() {
  margin = 10;
  positions = [
    [-total_width / 2 + margin, -shelf_depth / 2 + margin],
    [total_width / 2 - margin, -shelf_depth / 2 + margin],
    [-total_width / 2 + margin, shelf_depth / 2 - margin],
    [total_width / 2 - margin, shelf_depth / 2 - margin],
  ];

  for (pos = positions) {
    // Slot horizontal na parede traseira
    translate([pos[0], shelf_depth / 2 - 0.1, shelf_height - 10])
      rotate([-90, 0, 0]) {
        hull() {
          cylinder(d=mount_hole_d, h=wall_thickness + 0.2);
          translate([mount_slot_length, 0, 0])
            cylinder(d=mount_hole_d, h=wall_thickness + 0.2);
        }
      }
  }
}

module bottle_mount_holes(w, d) {
  margin = 10;
  positions = [
    [-w / 2 + margin, d / 2 - 0.1],
    [w / 2 - margin, d / 2 - 0.1],
  ];

  for (pos = positions) {
    translate([pos[0], pos[1], bottle_holder_h / 2])
      rotate([-90, 0, 0]) {
        hull() {
          cylinder(d=mount_hole_d, h=wall_thickness + 0.2);
          translate([mount_slot_length, 0, 0])
            cylinder(d=mount_hole_d, h=wall_thickness + 0.2);
        }
      }
  }
}

// ============================================================
// TAMPA DE ACRÍLICO (prateleira superior)
// ============================================================

module acrylic_lid() {
  margin = 8;
  post_positions = [
    [-total_width / 2 + margin, -shelf_depth / 2 + margin],
    [total_width / 2 - margin, -shelf_depth / 2 + margin],
    [-total_width / 2 + margin, shelf_depth / 2 - margin],
    [total_width / 2 - margin, shelf_depth / 2 - margin],
    [0, -shelf_depth / 2 + margin],
    [0, shelf_depth / 2 - margin],
  ];

  color("LightCyan", 0.4)
    translate([0, 0, shelf_height])
      difference() {
        rounded_rect(
          total_width - wall_thickness - 0.5,
          shelf_depth - wall_thickness - 0.5,
          acrylic_thickness, 3
        );

        // Furos M3
        for (pos = post_positions)
          translate([pos[0], pos[1], -0.1])
            cylinder(d=lid_screw_d + 0.3, h=acrylic_thickness + 0.2);

        // Slots de ventilação sobre cada bomba
        for (i = [0:num_stations - 1]) {
          x = -( (num_stations - 1) * station_spacing) / 2 + i * station_spacing;
          for (s = [-1, 0, 1])
            translate([x + s * 8 - 1.5, -shelf_depth / 4 - 12, -0.1])
              cube([3, 24, acrylic_thickness + 0.2]);
        }

        // Gravação IARA
        translate([0, shelf_depth / 4, acrylic_thickness - 0.3])
          linear_extrude(0.5)
            text(
              "I A R A", size=8, halign="center", valign="center",
              font="Liberation Sans:style=Bold"
            );
      }
}

// --- Postes para tampa ---
module lid_posts() {
  margin = 8;
  post_h = shelf_height - base_thickness - acrylic_thickness;
  positions = [
    [-total_width / 2 + margin, -shelf_depth / 2 + margin],
    [total_width / 2 - margin, -shelf_depth / 2 + margin],
    [-total_width / 2 + margin, shelf_depth / 2 - margin],
    [total_width / 2 - margin, shelf_depth / 2 - margin],
    [0, -shelf_depth / 2 + margin],
    [0, shelf_depth / 2 - margin],
  ];

  for (pos = positions) {
    translate([pos[0], pos[1], base_thickness]) {
      difference() {
        cylinder(d=lid_post_d, h=post_h);
        translate([0, 0, post_h - 6])
          cylinder(d=lid_screw_d, h=6.1);
      }
    }
  }
}

// ============================================================
// TEMPLATE 2D PARA CORTE LASER
// ============================================================

module acrylic_lid_2d() {
  margin = 8;
  post_positions = [
    [-total_width / 2 + margin, -shelf_depth / 2 + margin],
    [total_width / 2 - margin, -shelf_depth / 2 + margin],
    [-total_width / 2 + margin, shelf_depth / 2 - margin],
    [total_width / 2 - margin, shelf_depth / 2 - margin],
    [0, -shelf_depth / 2 + margin],
    [0, shelf_depth / 2 - margin],
  ];

  difference() {
    hull() {
      for (
        x = [
          -(total_width - wall_thickness) / 2 + 3,
          (total_width - wall_thickness) / 2 - 3,
        ]
      )
        for (
          y = [
            -(shelf_depth - wall_thickness) / 2 + 3,
            (shelf_depth - wall_thickness) / 2 - 3,
          ]
        )
          translate([x, y]) circle(r=3);
    }

    for (pos = post_positions)
      translate([pos[0], pos[1]]) circle(d=lid_screw_d + 0.3);

    for (i = [0:num_stations - 1]) {
      x = -( (num_stations - 1) * station_spacing) / 2 + i * station_spacing;
      for (s = [-1, 0, 1])
        translate([x + s * 8 - 1.5, -shelf_depth / 4 - 12])
          square([3, 24]);
    }
  }
}

// ============================================================
// MONTAGEM FINAL
// ============================================================

module full_assembly() {
  // --- Prateleira superior com bombas (impressão 3D) ---
  color("SteelBlue", 0.9)
    difference() {
      union() {
        pump_shelf();

        // Brackets das bombas (dentro da prateleira)
        for (i = [0:num_stations - 1]) {
          x = -( (num_stations - 1) * station_spacing) / 2 + i * station_spacing;
          translate([x, -shelf_depth / 2 + wall_thickness + pump_base_d / 2 + pump_clearance + 5, 0])
            pump_bracket();
        }

        // Zona de fiação
        wiring_mounts();

        // Postes para tampa
        lid_posts();
      }

      // Furos de montagem na parede
      shelf_mount_holes();
    }

  // Etiquetas externas
  pump_labels();

  // Tampa de acrílico
  acrylic_lid();

  // --- Prateleira inferior com garrafas ---
  bottle_offset_z = -(shelf_to_bottles + bottle_holder_h);
  translate(
    [
      0,
      -shelf_depth / 2 + wall_thickness + (bottle_diameter + bottle_wall * 2 + 10) / 2 + 3,
      bottle_offset_z,
    ]
  )
    color("SteelBlue", 0.8)
      bottle_shelf();

  // Ghost das garrafas para referência
  %ghost_bottles();
}

// --- Ghost das garrafas (transparente) ---
module ghost_bottles() {
  bottle_shelf_d = bottle_diameter + bottle_wall * 2 + 10;
  bottle_offset_z = -(shelf_to_bottles + bottle_holder_h);
  bottle_shelf_y = -shelf_depth / 2 + wall_thickness + bottle_shelf_d / 2 + 3;

  for (i = [0:num_stations - 1]) {
    x = -( (num_stations - 1) * station_spacing) / 2 + i * station_spacing;
    color("ForestGreen", 0.2)
      translate([x, bottle_shelf_y, bottle_offset_z + base_thickness])
        cylinder(d=bottle_diameter - 2, h=bottle_height);
  }
}

// --- Renderizar ---
full_assembly();

// ============================================================
// VARIANTES PARA FABRICAÇÃO
// ============================================================

// --- Imprimir prateleira superior ---
// pump_shelf(); // + brackets + mounts internos

// --- Imprimir prateleira inferior (garrafas) ---
// bottle_shelf();

// --- Exportar template 2D do acrílico ---
// acrylic_lid_2d();

// ============================================================
// NOTAS DE FABRICAÇÃO
// ============================================================
// PRATELEIRA SUPERIOR (Impressão 3D):
//   Material: PETG | Camada: 0.2mm | Preenchimento: 25%
//   Tempo estimado: ~8-12 horas
//
// PRATELEIRA INFERIOR (Impressão 3D):
//   Material: PETG | Camada: 0.2mm | Preenchimento: 25%
//   Tempo estimado: ~4-6 horas
//
// TAMPA (Corte laser):
//   Material: Acrílico transparente 3mm
//   Exportar acrylic_lid_2d() como DXF
//   Fixação: 6× parafusos M3×8mm
//
// MONTAGEM:
//   1. Fixar prateleira superior na parede (4 parafusos)
//   2. Fixar prateleira de garrafas abaixo (~20cm abaixo)
//   3. Montar bombas nos brackets
//   4. Conectar bornes na zona de fiação
//   5. Passar mangueiras pelos furos do fundo
//   6. Colocar tampa de acrílico
// ============================================================
