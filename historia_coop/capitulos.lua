-- Historia cooperativa: los capítulos.
-- Este archivo va en la carpeta historia_coop, junto a BT4decomp.exe.
-- IMPORTANTE: los dos jugadores tenéis que tener EXACTAMENTE el mismo archivo. Si cambia una sola letra,
-- el juego no deja unirse a la sala (así nunca jugáis capítulos distintos sin daros cuenta).
--
-- Cada capítulo es una tabla { ... } con estos campos:
--   id         nombre interno sin espacios (no lo cambies una vez jugado: se usa para guardar "completado")
--   saga       la saga, para agrupar
--   titulo     lo que sale en la lista
--   pareja     la pareja que os sugerimos (elegís libremente en la selección de personajes)
--   rivales    los dos rivales de la CPU:
--                personaje  número de personaje (la lista está al final de este archivo)
--                traje      0 = el normal (opcional)
--                nivel      0 muy fácil, 1 fácil, 2 normal, 3 difícil, 4 muy difícil
--                vida       en %, 100 = normal; más de 100 para jefes (10..400)
--                nombre     cómo se llama, para la ficha del capítulo (opcional)
--   escenario  número de escenario, o -1 para que el anfitrión lo elija en la pantalla de escenarios
--   musica     número de música, o -1 (el anfitrión la elige con el escenario)
--   tiempo     0 = 60 s, 1 = 90 s, 2 = 180 s, 3 = 240 s, 4 = sin límite
--   intro      el texto que se lee antes de crear la sala
--   cierre     el texto al ganar (cabe en unas 3 líneas)
--   derrota    el texto al perder (opcional)

return {
    {
        id = "saiyan_llegada",
        saga = "Saga Saiyajin",
        titulo = "La llegada de los saiyajin",
        pareja = "Goku y Piccoro",
        rivales = {
            { personaje = 80, nivel = 2, vida = 110, nombre = "Nappa" },
            { personaje = 29, nivel = 3, vida = 120, nombre = "Vegeta (Explorador)" },
        },
        escenario = -1, musica = -1, tiempo = 4,
        intro = "Dos saiyajin han aterrizado en la Tierra buscando las Bolas de Dragón. "
             .. "Nappa quiere divertirse y Vegeta solo espera. Esta vez no lucharéis por turnos: juntos.",
        cierre = "Los saiyajin se retiran malheridos. Vegeta jura que volverá más fuerte...",
        derrota = "Vegeta sonríe: «Ni juntos sois rival para la élite saiyajin». ¡Otra vez!",
    },
    {
        id = "freezer_ginyu",
        saga = "Saga Freezer",
        titulo = "Las Fuerzas Especiales Ginyu",
        pareja = "Gohan (Niño) y Krilin",
        rivales = {
            { personaje = 87, nivel = 2, vida = 120, nombre = "Recoome" },
            { personaje = 86, nivel = 3, vida = 120, nombre = "Ginyu" },
        },
        escenario = -1, musica = -1, tiempo = 4,
        intro = "Freezer ha llamado a sus mejores guerreros. Recoome posa, Ginyu planea algo. "
             .. "Aguantad hasta que llegue Goku... o acabad con ellos vosotros.",
        cierre = "¡Las Fuerzas Especiales Ginyu han caído! Freezer ya no tiene a quién enviar.",
        derrota = "«¡Pose de victoria!» Las Fuerzas Ginyu os han derrotado.",
    },
    {
        id = "freezer_final",
        saga = "Saga Freezer",
        titulo = "El tirano del universo",
        pareja = "Goku (Med) y Vegeta",
        rivales = {
            { personaje = 94, nivel = 3, vida = 175, nombre = "Freezer (Forma final)" },
            { personaje = 83, nivel = 2, vida = 100, nombre = "Zarbon (Transformado)" },
        },
        escenario = -1, musica = -1, tiempo = 4,
        intro = "Namek se desmorona. Freezer, en su forma final, no está solo: Zarbon le cubre las espaldas. "
             .. "Freezer tiene mucha más vida de lo normal. ¡Coordinaos!",
        cierre = "Freezer cae derrotado. Por fin el universo respira tranquilo... por ahora.",
        derrota = "Freezer ríe mientras Namek explota. Habéis caído.",
    },
    {
        id = "androides_gemelos",
        saga = "Saga de los Androides",
        titulo = "Los gemelos artificiales",
        pareja = "Trunks y Piccoro (Fin)",
        rivales = {
            { personaje = 101, nivel = 3, vida = 130, nombre = "Androide n.º 17" },
            { personaje = 102, nivel = 3, vida = 130, nombre = "Androide n.º 18" },
        },
        escenario = -1, musica = -1, tiempo = 4,
        intro = "Los androides 17 y 18 se han despertado y no se cansan nunca. "
             .. "No dejéis que os separen: dos contra dos, energía infinita contra trabajo en equipo.",
        cierre = "Los androides se retiran. Ha sido duro, pero el futuro aún tiene esperanza.",
        derrota = "18 bosteza: «¿Ya está?». Los androides han ganado.",
    },
    {
        id = "cell_juegos",
        saga = "Saga Cell",
        titulo = "Los Juegos de Cell",
        pareja = "Gohan (Joven) y Goku (Fin)",
        rivales = {
            { personaje = 107, nivel = 4, vida = 200, nombre = "Cell (Super Perfecto)" },
            { personaje = 109, nivel = 2, vida = 80, nombre = "Cell Jr." },
        },
        escenario = -1, musica = -1, tiempo = 4,
        intro = "Cell ha organizado su torneo y no piensa jugar limpio: ha traído a uno de sus hijos. "
             .. "Cell Super Perfecto es el jefe final, con el doble de vida. ¡Kamehameha padre e hijo!",
        cierre = "¡Cell ha sido derrotado! La Tierra está a salvo gracias a vuestro trabajo en equipo.",
        derrota = "Cell se ríe: «¡Perfecto!». La Tierra está perdida... de momento.",
    },
}

-- Algunos números de personaje (lista completa: el visor de combos o pregúntale a Claude):
--   0 Goku (Inicio)   1 Goku (Med)   3 Goku (Fin)   13 Gohan (Niño)   14 Gohan (Joven)   17 Gohan
--   22 Piccoro (Inicio)   23 Piccoro (Fin)   25 Krilin   26 Yamcha   27 Tenshinhan   29 Vegeta (Explorador)
--   31 Vegeta   39 Trunks (Espada)   41 Trunks   77 Raditz   80 Nappa   82 Zarbon   83 Zarbon (Transformado)
--   84 Dodoria   86 Ginyu   87 Recoome   88 Burter   89 Jeice   91..95 Freezer (1.ª forma .. 100 %)
--   96 Mecha Freezer   100..103 Androides 16..19   105 Cell (1.ª forma)   106 Cell (2.ª forma)
--   107 Cell (Super Perfecto)   108 Cell Max   109 Cell Jr.
