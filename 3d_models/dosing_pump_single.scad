// ============================================================
// IARA - Suporte Individual para 1 Bomba Dosadora (Prime)
// Arquivo: dosing_pump_single.scad
// Descrição: Suporte para 1 bomba + 1 garrafa Seachem 500ml
//            Layout vertical: bomba em cima, garrafa embaixo
//            Montagem na parede
// ============================================================

$fn = 60;

// ============================================================
// PARÂMETROS
// ============================================================

// -- Garrafa Seachem 500ml (dimensões reais) --
bottle_diameter = 67; // mm (65mm real + 2mm folga)
bottle_height = 185; // mm (18.5cm)
bottle_holder_h = 45; // mm - altura do anel
bottle_wall = 2.5; // mm

// -- Bomba dosadora peristáltica (dimensões reais) --
pump_base_d = 54; // mm (diâmetro da base)
pump_body_d = 48; // mm (diâmetro do corpo)
pump_height = 46.3; // mm (altura total)
pump_mount_holes = 50; // mm (distância entre furos)
pump_mount_hole_d = 4; // mm (Ø furos)
pump_clearance = 2; // mm

// -- Estrutura --
wall = 3; // mm
base_t = 4; // mm - espessura da base
bracket_outer_d = pump_base_d + pump_clearance * 2 + wall * 2;

// -- Backplate (placa traseira para parede) --
bp_width = bracket_outer_d + 20; // mm
bp_height = pump_height + 20; // mm
bp_thickness = 4; // mm
mount_hole_d = 5; // mm
mount_slot_l = 8; // mm

// -- Conector P4 --
p4_hole_d = 8; // mm

// -- Prateleira da garrafa --
shelf_width = bottle_diameter + bottle_wall * 2 + 16;
shelf_depth = bottle_diameter + bottle_wall * 2 + 10;

// ============================================================
// MÓDULOS
// ============================================================

// --- Bracket circular da bomba ---
module pump_bracket() {
  cradle_h = pump_height * 0.6;

  difference() {
    union() {
      // Base circular
      cylinder(d=bracket_outer_d, h=base_t);

      // Berço cilíndrico
      difference() {
        cylinder(d=bracket_outer_d, h=cradle_h);
        translate([0, 0, base_t])
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

    // Furos de montagem (2 orelhas)
    for (dx = [-pump_mount_holes / 2, pump_mount_holes / 2])
      translate([dx, 0, -0.1])
        cylinder(d=pump_mount_hole_d, h=base_t + 0.2);

    // Abertura frontal para mangueiras
    translate([-pump_body_d / 3, -bracket_outer_d / 2 - 1, base_t])
      cube([pump_body_d * 2 / 3, bracket_outer_d / 3, cradle_h]);

    // Abertura traseira para fios
    translate([-10, bracket_outer_d / 4, base_t + 5])
      cube([20, bracket_outer_d / 2, 15]);
  }
}

// --- Backplate (placa de montagem na parede) ---
module backplate() {
  difference() {
    union() {
      // Placa traseira com cantos arredondados
      hull() {
        for (x = [-bp_width / 2 + 4, bp_width / 2 - 4])
          for (z = [4, bp_height - 4])
            translate([x, 0, z])
              rotate([-90, 0, 0])
                cylinder(r=4, h=bp_thickness);
      }

      // Base para a bomba (projeção frontal)
      translate([0, bp_thickness, bp_height / 2])
        rotate([-90, 0, 0])
          cylinder(d=bracket_outer_d + 4, h=base_t);
    }

    // Furos de montagem na parede (slots)
    for (x = [-bp_width / 2 + 10, bp_width / 2 - 10]) {
      for (z = [12, bp_height - 12]) {
        translate([x, -0.1, z])
          rotate([-90, 0, 0])
            hull() {
              cylinder(d=mount_hole_d, h=bp_thickness + 0.2);
              translate([mount_slot_l, 0, 0])
                cylinder(d=mount_hole_d, h=bp_thickness + 0.2);
            }
      }
    }

    // Furo para cabo P4
    translate([bp_width / 2 - 15, -0.1, bp_height / 2])
      rotate([-90, 0, 0])
        cylinder(d=p4_hole_d, h=bp_thickness + 0.2);

    // Furos para mangueiras
    translate([-10, -0.1, bp_height / 2 - 15])
      rotate([-90, 0, 0])
        cylinder(d=6, h=bp_thickness + 0.2);
    translate([10, -0.1, bp_height / 2 - 15])
      rotate([-90, 0, 0])
        cylinder(d=6, h=bp_thickness + 0.2);
  }

  // Etiqueta "PRIME"
  translate([0, -0.1, bp_height - 8])
    rotate([90, 0, 0])
      linear_extrude(0.6)
        text(
          "PRIME", size=6, halign="center", valign="center",
          font="Liberation Sans:style=Bold"
        );
}

// --- Holder da garrafa ---
module bottle_holder_single() {
  difference() {
    union() {
      // Base com cantos arredondados
      hull() {
        for (x = [-shelf_width / 2 + 4, shelf_width / 2 - 4])
          for (y = [-shelf_depth / 2 + 4, shelf_depth / 2 - 4])
            translate([x, y, 0])
              cylinder(r=4, h=base_t);
      }

      // Paredes laterais
      difference() {
        hull() {
          for (x = [-shelf_width / 2 + 4, shelf_width / 2 - 4])
            for (y = [-shelf_depth / 2 + 4, shelf_depth / 2 - 4])
              translate([x, y, 0])
                cylinder(r=4, h=bottle_holder_h);
        }
        translate([0, 0, base_t])
          hull() {
            for (x = [-shelf_width / 2 + wall + 4, shelf_width / 2 - wall - 4])
              for (y = [-shelf_depth / 2 + wall + 4, shelf_depth / 2 - wall - 4])
                translate([x, y, 0])
                  cylinder(r=4, h=bottle_holder_h + 1);
          }
      }

      // Anel interno para a garrafa
      difference() {
        cylinder(d=bottle_diameter + bottle_wall * 2, h=bottle_holder_h);
        translate([0, 0, base_t])
          cylinder(d=bottle_diameter, h=bottle_holder_h);
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
    }

    // Dreno no fundo
    translate([0, 0, -0.1])
      cylinder(d=18, h=base_t + 0.2);

    // Fenda frontal para encaixe/retirada
    translate([-bottle_diameter / 4, -shelf_depth / 2 - 1, base_t + 12])
      cube([bottle_diameter / 2, wall + 2, bottle_holder_h]);

    // Furos de montagem na parede (traseiro)
    for (x = [-shelf_width / 2 + 8, shelf_width / 2 - 8]) {
      translate([x, shelf_depth / 2 - 0.1, bottle_holder_h / 2])
        rotate([-90, 0, 0])
          hull() {
            cylinder(d=mount_hole_d, h=wall + 0.2);
            translate([mount_slot_l, 0, 0])
              cylinder(d=mount_hole_d, h=wall + 0.2);
          }
    }
  }

  // Etiqueta "PRIME"
  translate([0, -shelf_depth / 2 - 0.1, bottle_holder_h / 2])
    rotate([90, 0, 0])
      linear_extrude(0.6)
        text(
          "PRIME", size=5, halign="center", valign="center",
          font="Liberation Sans:style=Bold"
        );
}

// ============================================================
// MONTAGEM COMPLETA
// ============================================================

module full_single_pump() {
  // Backplate com bracket (montado na parede)
  color("SteelBlue", 0.9) {
    backplate();

    // Bracket da bomba na backplate
    translate([0, bp_thickness + base_t, bp_height / 2])
      rotate([-90, 0, 0])
        pump_bracket();
  }

  // Prateleira da garrafa (abaixo)
  gap = 25; // espaço para mangueiras
  color("SteelBlue", 0.8)
    translate([0, bp_thickness / 2 + shelf_depth / 2 - 5, -gap - bottle_holder_h])
      bottle_holder_single();

  // Ghost da garrafa
  %translate([0, bp_thickness / 2 + shelf_depth / 2 - 5, -gap - bottle_holder_h + base_t])
    color("ForestGreen", 0.2)
      cylinder(d=bottle_diameter - 2, h=bottle_height);

  // Ghost da bomba
  %translate([0, bp_thickness + base_t, bp_height / 2])
    rotate([-90, 0, 0])
      color("DimGray", 0.3)
        cylinder(d=pump_body_d, h=pump_height);
}

// --- Renderizar ---
full_single_pump();

// ============================================================
// VARIANTES PARA IMPRESSÃO
// ============================================================

// --- Apenas backplate + bracket ---
// backplate();
// translate([0, bp_thickness + base_t, bp_height/2])
//     rotate([-90,0,0]) pump_bracket();

// --- Apenas holder da garrafa ---
// bottle_holder_single();

// ============================================================
// NOTAS DE FABRICAÇÃO
// ============================================================
// Material: PETG
// Camada: 0.2mm | Preenchimento: 25%
// Tempo estimado: ~4-5 horas total
//
// MONTAGEM:
//   1. Fixar backplate na parede (4 parafusos)
//   2. Fixar prateleira da garrafa ~25mm abaixo
//   3. Encaixar bomba no bracket
//   4. Conectar mangueiras e cabo P4
// ============================================================
