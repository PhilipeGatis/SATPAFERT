// ============================================================
// IARA - Suporte para 4 Bombas Dosadoras + Garrafas Seachem 500ml
// Versão: Corte a Laser (Acrílico ou MDF 3mm)
// ============================================================

$fn = 60;

// -- Parâmetros de Material e Tolerância --
mat_t = 3;       // mm - Espessura do material
tol = 0.15;      // mm - Folga para encaixes (aumentar furos por tol*2 para encaixe macio)

// -- Dimensões Gerais do Rack --
num_stations = 4;
station_spacing = 80;
rack_w = 320;    // Largura interna clara
rack_h = 350;    // Altura total
depth = 80;      // Profundidade (frente a trás) - Otimizado para caber tudo

// -- Posições Z das Prateleiras --
bot_z = 20;      // Base inferior (apoio de garrafas)
col_z = 80;      // Anel (colar) de retenção das garrafas
pmp_z = 290;     // Prateleira das bombas (Deixa 203mm de vão livre para garrafas de 185)

// -- Dimensões dos Componentes --
bottle_d = 64;   // Diâmetro real da garrafa Seachem 500ml
bottle_hole_d = 65; // Furo final exato de 65mm para abraçar a garrafa

pump_body_d = 48;
pump_holes = 47.5;     // Entre eixos de fixação
pump_mount_hole_d = 3; // Diâmetro do parafuso
hose_hole_d = 8;
tab_w = 15;      // Largura padrão das abas (tabs) de fixação

// ============================================================
// MÓDULOS DE COMPONENTES 2D (Para Corte Laser)
// ============================================================

// --- Centralizar furos de fixação (Keyhole) ---
module keyhole() {
    hull() {
        translate([0, 4]) circle(d=8);    // Passagem da cabeça do parafuso
        translate([0, -2]) circle(d=4.5); // Descanso do corpo do parafuso
    }
}

// Fica posicionado em Y=0. Cobre apenas a parte de trás das bombas (z = 280 a rack_h).
module back_panel_2d() {
    union() {
        difference() {
            // Corpo Principal: z de 280 a rack_h
            translate([-rack_w/2 - mat_t, 280]) 
                square([rack_w + 2*mat_t, rack_h - 280]);
            
            // Furos de fixação na parede (Keyholes)
            for (x = [-140, 140]) {
                translate([x, 320]) keyhole();
            }
            
            // Slots Verticais: para Painéis Laterais (x = -161.5, +161.5)
            for (x = [-rack_w/2 - mat_t/2, rack_w/2 + mat_t/2]) {
                for (z = [310]) {
                    translate([x, z]) 
                        square([mat_t + tol*2, tab_w + tol*2], center=true);
                }
            }
            
            // Slots Verticais: para Painel Divisório Central (x = 0)
            for (z = [310]) {
                translate([0, z]) 
                    square([mat_t + tol*2, tab_w + tol*2], center=true);
            }
            
            // Slots Horizontais: para as abas da Prateleira das Bombas
            for (z = [pmp_z]) {
                for (x = [-80, 80]) {
                    translate([x, z]) 
                        square([tab_w + tol*2, mat_t + tol*2], center=true);
                }
            }
        }
        
        // Tabs Superiores: para a Tampa (Top Lid)
        for (x = [-80, 80]) {
            translate([x - tab_w/2, rack_h])
                square([tab_w, mat_t]);
        }
    }
    
    // Gravação Logo
    color("red")
    translate([0, rack_h - 25])
        text("I A R A", size=10, halign="center", valign="center", font="Liberation Sans:style=Bold");
}

// ============================================================
// 2. PAINÉIS VERTICAIS (Laterais e Central)
// ============================================================
// Ficam posicionados no eixo YZ da montagem. 
// A profundidade está no eixo X (0 a depth), a altura está no Y (0 a rack_h).
module vertical_panel_2d(is_mid=false) {
    union() {
        difference() {
            // Corpo Principal
            square([depth, rack_h]);
            
            // Tratamento das interações com Prateleiras:
            if (is_mid) {
                // Painel Central (Divisória) usa 'Halving Joints'
                for (z = [bot_z, pmp_z]) { // <-- Removido col_z para economizar placa
                    // Recorte partindo do centro até a frente (depth/2 até depth)
                    translate([depth/2, z - mat_t/2 - tol]) 
                        square([depth/2 + 1, mat_t + tol*2]);
                }
            } else {
                // Painéis Laterais (Esquerdo/Direito) usam slots normais
                for (z = [bot_z, pmp_z]) {
                    for (x = [20, 60]) { // Movido levemente pra caber no depth=80
                        translate([x - tab_w/2 - tol, z - mat_t/2 - tol]) 
                            square([tab_w + tol*2, mat_t + tol*2]);
                    }
                }
            }
            
            // Furo central de 20mm para condução Mestre dos Fios/Cabos
            // Fica localizado na "caixa elétrica" que se forma entre a placa de bombas e a tampa
            translate([depth/2, rack_h - 30]) circle(d=20);
        }
        
        // Tabs Traseiros: saem do zero para a esquerda e encaixam no Painel Traseiro (agora apenas na área superior)
        for (z = [310]) {
            translate([-mat_t, z - tab_w/2])
                square([mat_t, tab_w]);
        }
        
        // Tabs Superiores: para a Tampa (Top Lid)
        for (x = [20, 60]) { 
            translate([x - tab_w/2, rack_h])
                square([tab_w, mat_t]);
        }
    }
}

// ============================================================
// 3. PRATELEIRAS (Base genérica e específicas)
// ============================================================
// Posicionadas no eixo XY da montagem. X é a largura, Y é a profundidade.

module shelf_base_2d(has_back_tabs=false) {
    union() {
        difference() {
            // Corpo principal da prateleira (cobre exato o espaço interno entre os painéis laterais)
            translate([-rack_w/2, 0]) 
                square([rack_w, depth]);
            
            // Halving notch para o painel central:
            // A prateleira TEM NOTCH NA PARTE TRASEIRA para cruzar com o Painel Central.
            // Posição X=0, do fundo Y=0 até o meio Y=depth/2
            translate([-mat_t/2 - tol, -0.1]) 
                square([mat_t + tol*2, depth/2 + tol + 0.1]);
        }
        
        if (has_back_tabs) {
            // Tabs Traseiros: encaixam no Painel Traseiro
            for (x = [-80, 80]) {
                translate([x - tab_w/2, -mat_t]) 
                    square([tab_w, mat_t]);
            }
        }
        
        // Tabs Laterais: encaixam nas slots dos Painéis Laterais
        for (y = [20, 60]) {
            // Aba lado esquerdo
            translate([-rack_w/2 - mat_t, y - tab_w/2])
                square([mat_t, tab_w]);
            // Aba lado direito
            translate([rack_w/2, y - tab_w/2])
                square([mat_t, tab_w]);
        }
    }
}

// --- 3.1: Prateleira de Chão (Pouso da garrafa) ---
module shelf_bot_2d() {
    shelf_base_2d();
}

// --- 3.2: Prateleira/Colar Intermediário (Alinhamento da garrafa) ---
module shelf_col_2d() {
    difference() {
        shelf_base_2d();
        // Furo grande para passar a garrafa inteira (68mm)
        for (x = [-120, -40, 40, 120]) {
            translate([x, depth/2]) circle(d=bottle_hole_d);
        }
    }
}

// --- 3.3: Prateleira das Bombas (Motor e fixação) ---
module shelf_pmp_2d() {
    difference() {
        shelf_base_2d(has_back_tabs=true);
        for (x = [-120, -40, 40, 120]) {
            // Furos de fixação da bomba (No centro da prateleira sobre a garrafa)
            translate([x - pump_holes/2, depth/2]) circle(d=pump_mount_hole_d);
            translate([x + pump_holes/2, depth/2]) circle(d=pump_mount_hole_d);
            
            // Vão amplo para passagem das mangueiras (do centro até a borda frontal)
            translate([x, depth/2 + 24]) square([40, 48], center=true);
        }
    }
    
    // Gravações Leds / Etiquetas
    color("red")
    for (i = [0:num_stations-1]) {
        x = -120 + i*80;
        translate([x, depth - 8])
            text(str("CH ", i+1), size=5, halign="center", valign="center", font="Liberation Sans:style=Bold");
    }
}

// --- 3.4: Tampa Superior (Ocultar fios) ---
module top_lid_2d() {
    difference() {
        // Tampa cobrindo todo o topo, incluindo encostar 100% no painel traseiro e painéis laterais
        translate([-rack_w/2 - mat_t, -mat_t]) 
            square([rack_w + 2*mat_t, depth + mat_t]);
            
        // Furos para fixar os tabs do Painel Traseiro
        for (x = [-80, 80]) { 
            translate([x, -mat_t/2]) 
                square([tab_w + tol*2, mat_t + tol*2], center=true);
        }
        
        // Furos para fixar os tabs dos Painéis Verticais
        for (x = [-rack_w/2 - mat_t/2, 0, rack_w/2 + mat_t/2]) {
            for (y = [20, 60]) {
                translate([x, y]) 
                    square([mat_t + tol*2, tab_w + tol*2], center=true);
            }
        }
    }
    
    // Opcional: Gravação no topo
    color("red")
    translate([0, depth/2])
        text("I A R A", size=10, halign="center", valign="center", font="Liberation Sans:style=Bold");
}

// ============================================================
// MONTAGEM 3D VIRTUAL (Para validação visual e render)
// ============================================================

module full_assembly() {
    // 1. Painel Traseiro
    color("SteelBlue", 0.6)
    translate([0, 0, 0])
    rotate([90, 0, 0])
    back_panel_2d();
    
    // 2. Painel Lateral Esquerdo
    color("DarkCyan", 0.6)
    translate([-rack_w/2 - mat_t, 0, 0])
    rotate([0, 0, 90])
    rotate([90, 0, 0])
    vertical_panel_2d(is_mid=false);
    
    // 3. Painel Lateral Direito
    color("DarkCyan", 0.6)
    translate([rack_w/2, 0, 0])
    rotate([0, 0, 90])
    rotate([90, 0, 0])
    vertical_panel_2d(is_mid=false);
    
    // 4. Painel Central (Divisória)
    color("CadetBlue", 0.6)
    translate([-mat_t/2, 0, 0])
    rotate([0, 0, 90])
    rotate([90, 0, 0])
    vertical_panel_2d(is_mid=true);
    
    // 5. Prateleiras e Tampa
    color("Goldenrod", 0.7) {
        translate([0, 0, bot_z]) shelf_bot_2d();
        // Não há mais collar para caber na chapa 1 unica:
        // translate([0, 0, col_z]) shelf_col_2d();
        translate([0, 0, pmp_z]) shelf_pmp_2d();
    }
    
    // 6. Tampa Superior (Lid)
    color("LightYellow", 0.6)
    translate([0, 0, rack_h])
    top_lid_2d();
    
    // 7. Modelo de Referência das Garrafas (Fantasma)
    %for (x = [-120, -40, 40, 120]) {
        translate([x, depth/2, bot_z + mat_t])
            cylinder(d=bottle_d, h=185);
    }
}

// ============================================================
// TEMPLATE 2D PARA CORTE ("Ninho") - Exportar como DXF
// ============================================================

module all_2d() {
    m = 10; // Margem segura das bordas da placa de MDF
    s = 5;  // Espaçamento entre o corte das peças
    
    // ==========================================
    // CHAPA COMPLETA UNICA: 40x60cm (400x600mm)
    // ==========================================
    // Área da chapa (Ghost)
    %square([600, 400]);
    
    // --- LADO ESQUERDO: Peças horizontais (centralizadas em X=0 localmente) ---
    // Offset para a esquerda pra respeitar a margem m: x = largura_peça/2 + m
    left_x = (rack_w + 2*mat_t)/2 + m; 
    translate([left_x, m]) {
        // 1. Back Panel (Altura útil de 70)
        translate([0, -280]) back_panel_2d();
        
        // 2. Shelf Base
        y_sh1 = 70 + s;
        translate([0, y_sh1]) shelf_bot_2d();
        
        // 3. Shelf Pumps
        y_sh2 = y_sh1 + depth + s;
        translate([0, y_sh2]) shelf_pmp_2d();
        
        // 4. Top Lid
        y_lid = y_sh2 + depth + s + mat_t;
        translate([0, y_lid]) top_lid_2d();
    }
    
    // --- LADO DIREITO: Peças verticais ---
    // Começa na direita exata do bloco esquerdo
    right_x = left_x + (rack_w + 2*mat_t)/2 + s;
    translate([right_x, m]) {
        translate([0, 0]) 
            vertical_panel_2d(is_mid=false);
            
        translate([depth + s, 0]) 
            vertical_panel_2d(is_mid=false);
            
        translate([2*(depth + s), 0]) 
            vertical_panel_2d(is_mid=true);
    }
}


// --- Modo de exibição: ---
// Comente e descomente para trocar o que está sendo gerado:

//full_assembly();  // <---- Visualização Completa em 3D
all_2d();      // <---- Visualização 2D Plana (Para exportar para DXF)
