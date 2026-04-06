// Suporte em Moldura para Placas de Titânio de Eletrólise - Projeto IARA
// Design "Window Frame" com encaixe por pressão e montagem inferior para ventosa
// Unidades: Milímetros (mm)

/* [Dimensões das Placas de Titânio] */
// Largura da placa (mm). Ex: 30 para a placa de 3x4
plate_width = 30.0; 
// Espessura da chapa de titânio (mm)
plate_thickness = 1.0; 
// Distância exata desejada entre as duas placas (mm)
gap_between_plates = 3.0; 

/* [Dimensões da Moldura (Frame)] */
// Altura da moldura plástica.
// Com 36mm, uma placa de 40mm fica um tiquinho exposta no topo (embora agora os fios passarão por baixo)
frame_height = 36.0; 

// O quanto a moldura cobre a esquerda e a direita da placa (mm)
side_overlap = 3.5; 

// Espessura do "chão" da moldura. Fica na parte inferior para ter massa pro parafuso da ventosa.
bottom_bar_height = 8.0; 

// Espessura das paredes externas (Frente e Trás)
wall_thickness = 2.5; 

// Tolerância extra no rasgo (slot) pro encaixe de "presilha por pressão" (fricção)
tolerance = 0.1; 

/* [Furos para os Fios de Contato (Na Base Inferior)] */
// Diâmetro dos furos para passar os fios (para poder fixar eles por baixo das placas)
wire_hole_d = 3.5; 

/* [Montagem da Ventosa na Face de Baixo] */
// 1 = Gerar um Pino (para encaixar em uma ventosa de borracha com furo)
// 2 = Gerar um Furo (para ventosas que possuem parafuso ou rosca embutida)
ventosa_mount_type = 2; 

// --- Configurações se usar PINO (ventosa_mount_type = 1) ---
ventosa_neck_d = 5.5;  
ventosa_head_d = 8.5;  
ventosa_neck_h = 4.0;  

// --- Configurações se usar FURO/PARAFUSO (ventosa_mount_type = 2) ---
// Exemplo: 4.0 para furar livre p/ M4, ou 3.8mm para roscar apertado direto no plástico M4.
ventosa_screw_hole_d = 3.8; 
ventosa_screw_hole_depth = 6.0;

/* [Hidden] */
$fn = 60;
slot_w = plate_thickness + tolerance;

total_w = plate_width + (side_overlap * 2);
total_d = gap_between_plates + (slot_w * 2) + (wall_thickness * 2);
total_h = frame_height;

module ventosa_peg() {
    // Pescoço
    cylinder(h=ventosa_neck_h + 0.1, d=ventosa_neck_d);
    // Cabeça
    translate([0, 0, ventosa_neck_h])
        cylinder(h=2.5, d1=ventosa_head_d, d2=ventosa_neck_d - 1.0);
}

module frame() {
    difference() {
        // Corpo Base (Bloco sólido dimensionado com as medidas externas)
        translate([0, 0, total_h/2])
            cube([total_w, total_d, total_h], center=true);
            
        // Recorte da Janela Central (Deixa o centro das placas 100% exposto à água!)
        translate([0, 0, bottom_bar_height + total_h/2])
            cube([plate_width - (side_overlap * 2), total_d + 2.0, total_h], center=true);
            
        // Fenda para a Primeira Placa (Frontal)
        translate([0, (gap_between_plates/2) + (slot_w/2), bottom_bar_height + total_h/2])
             cube([plate_width + tolerance, slot_w, total_h + 2.0], center=true);
             
        // Fenda para a Segunda Placa (Traseira)
        translate([0, -(gap_between_plates/2) - (slot_w/2), bottom_bar_height + total_h/2])
             cube([plate_width + tolerance, slot_w, total_h + 2.0], center=true);
             
        // Chanfro Superior suave nas fendas
        translate([0, (gap_between_plates/2) + (slot_w/2), total_h + 0.5])
            cube([plate_width + 0.5, slot_w + 1.2, 2.0], center=true);
        translate([0, -(gap_between_plates/2) - (slot_w/2), total_h + 0.5])
            cube([plate_width + 0.5, slot_w + 1.2, 2.0], center=true);

        // --- FURO CENTRAL DA VENTOSA NA FACE DE BAIXO ---
        if (ventosa_mount_type == 2) {
            // Um furo cilíndrico na parte de baixo para rosquear o parafuso da ventosa
            translate([0, 0, -1])
                cylinder(h=ventosa_screw_hole_depth + 1.0, d=ventosa_screw_hole_d);
        }

        // --- BURACOS PARA PUXAR O FIO DE CONTATO ---
        // Para evitar curto-circuito, fiz um buraco na ESQUERDA indo para a Placa FRONTAL
        // e um buraco na DIREITA indo para a Placa TRASEIRA. 
        // Desse forma os fios ficam bem separados.
        
        // Furo Esquerdo (Vai da base e entra exatamente no vão/rasgo da Placa 1)
        translate([-plate_width/3, (gap_between_plates/2) + (slot_w/2), -1])
            cylinder(h=bottom_bar_height + 2.0, d=wire_hole_d);
            
        // Furo Direito (Vai da base e entra exatamente no vão/rasgo da Placa 2)
        translate([plate_width/3, -(gap_between_plates/2) - (slot_w/2), -1])
            cylinder(h=bottom_bar_height + 2.0, d=wire_hole_d);
    }
    
    // Pino Adicionado se for ventosa tipo pino
    if (ventosa_mount_type == 1) {
        // Apontando para baixo (-Z)
        translate([0, 0, 0])
            rotate([180, 0, 0])
            ventosa_peg();
    }
}

// Renderizar (Como os furos estão embaixo, a impressão sai limpa com uma pequena ponte)
frame();
