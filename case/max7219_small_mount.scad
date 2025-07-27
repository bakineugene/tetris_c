// === Параметры ===
plate_length = 32.2;       // Длина платы (мм)
plate_width = 32.2;        // Ширина платы (мм)
hole_diameter = 3.5;       // Диаметр монтажного отверстия (мм)
peg_height = 3;          // Высота штырьков (мм)
peg_diameter = 3.2;      // Диаметр штырька (мм) — немного меньше отверстия
base_thickness = 3;      // Толщина подложки (мм)
peg_offset = 1.3 + hole_diameter  / 2.;          // Отступ отверстий от края платы (мм)
peg_offset_2 = 4.2 + hole_diameter  / 2.;          // Отступ отверстий от края платы (мм)

// === Основной вызов ===
generate_mount();

// === Модули ===

module generate_mount() {
    difference() {
        // Подложка
        cube([plate_length, plate_width, base_thickness]);
    }

    // Штырьки
    for (pos = hole_positions()) {
        translate([pos[0], pos[1], base_thickness])
            peg();
    }
}

// Позиции отверстий (в углах)
function hole_positions() = [
    [peg_offset, peg_offset_2],
    [plate_length - peg_offset, peg_offset_2],
    [plate_length - peg_offset, plate_width - peg_offset_2],
    [peg_offset, plate_width - peg_offset_2]
];

// Штырёк (цилиндр)
module peg() {
    cylinder(h = peg_height, d = peg_diameter, $fn=40);
}
