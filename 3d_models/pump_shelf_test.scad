// ============================================================
// IARA - TESTE DE FURAÇÃO: 1 Estação de Bomba Dosadora
// Versão: Corte a Laser (MDF 3mm)
// ============================================================
// Corte este arquivo para validar:
//   - Posição dos furos M3 de fixação da bomba
//   - Largura e posição do vão de passagem de mangueira
//   - Dimensão geral da prateleira (largura x profundidade)
// ============================================================

$fn = 60;

// -- Parâmetros (idênticos ao projeto principal) --
mat_t = 3;
tol   = 0.15;

// Dimensões da peça de teste (1 estação)
piece_w = 80;    // Largura de 1 estação
depth   = 100;   // Profundidade (frente a trás)

// Bomba
pump_holes       = 47.5; // Entre-eixos dos furos de fixação (mm)
pump_mount_hole_d = 3;   // Diâmetro do furo M3

// Vão de mangueira
hose_slot_w    = 40; // Largura do vão
hose_slot_back = 23; // Quanto o vão avança para a traseira (atrás dos furos M3)

// -- Peça Principal --
difference() {
    // Base retangular de 1 estação
    square([piece_w, depth], center=true);
    
    // ---- Furos M3 de fixação da bomba ----
    // Centralizados em Y=0 (meio da peça), espaçados em pump_holes
    translate([-pump_holes/2, 0]) circle(d=pump_mount_hole_d);
    translate([ pump_holes/2, 0]) circle(d=pump_mount_hole_d);
    
    // ---- Vão de passagem da mangueira ----
    // Vai da traseira (20mm atrás dos furos) até a borda frontal
    slot_h   = depth/2 + hose_slot_back;  // Total: 50 + 20 = 70mm
    slot_cy  = -hose_slot_back/2 + depth/4; // Centro do slot no eixo Y
    translate([0, slot_cy])
        square([hose_slot_w, slot_h], center=true);
}

// -- Anotações e cotas (aparecem no preview, não são cortadas) --
%color("blue") {
    // Linha de referência dos furos M3 (Y=0 = centro da prateleira)
    translate([-piece_w/2 - 5, 0]) text("Y=0 (centro)", size=3, valign="center");
    
    // Cota da largura entre furos
    translate([0, -depth/2 - 8]) {
        text(str("Entre-eixos: ", pump_holes, "mm"), size=3.5, halign="center");
    }
    
    // Indicação da frente
    translate([0, depth/2 + 3]) {
        text("FRENTE (mangueira sai aqui)", size=3, halign="center");
    }
    
    // Indicação da traseira
    translate([0, -depth/2 - 3]) {
        text("TRASEIRA", size=3, halign="center");
    }
}

// -- Renderizar --
// Apenas a peça 2D acima. Exporte como DXF para cortar no laser.
