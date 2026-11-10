-- Historia cooperativa: los capítulos.
-- Este archivo va en la carpeta historia_coop, junto a BT4decomp.exe.
-- IMPORTANTE: los dos jugadores tenéis que tener EXACTAMENTE el mismo archivo (las actualizaciones automáticas
-- ya se encargan). Si cambia una sola letra, el juego no deja unirse a la sala.
--
-- Los capítulos se juegan en orden: cada uno que superáis desbloquea el siguiente.
--
-- Cada capítulo es una tabla { ... } con estos campos:
--   id         nombre interno sin espacios (no lo cambies una vez jugado: se usa para guardar "completado")
--   saga       la saga, para agrupar
--   titulo     lo que sale en la lista
--   pareja     la pareja que os sugerimos (elegís libremente en la selección de personajes)
--   rivales    los rivales que empiezan en el combate (de 1 a 4; la sala es de 2 jugadores + estos):
--                personaje  número de personaje (la lista está al final de este archivo)
--                traje      0 = el normal (opcional)
--                nivel      0 muy fácil, 1 fácil, 2 normal, 3 difícil, 4 muy difícil
--                vida       en %, 100 = normal; más de 100 para jefes (10..400)
--                nombre     cómo se llama, para la ficha del capítulo (opcional)
--   refuerzos  (opcional) más rivales: cuando cae uno de los que están luchando, entra el siguiente en su
--              lugar. Mismos campos que los rivales, y además:
--                detras     detrás de qué rival entra (1 = el primero de «rivales»...). Sin él, se reparten
--                           por turnos. Máximo 4 detrás de cada rival.
--              El capítulo se gana cuando no queda ninguno en pie.
--   escenario  número de escenario, o -1 para que el anfitrión lo elija en la pantalla de escenarios
--   musica     número de música, o -1 (el anfitrión la elige con el escenario)
--   tiempo     0 = 60 s, 1 = 90 s, 2 = 180 s, 3 = 240 s, 4 = sin límite
--   intro      el texto que se lee antes de crear la sala
--   cierre     el texto al ganar (cabe en unas 3 líneas)
--   derrota    el texto al perder (opcional)
--
-- La dificultad viene del número de enemigos y de la vida de los jefes, no de que la CPU esquive más:
-- los rivales normales van en nivel 2 y los jefes en 3.

return {
    -- ------------------------------------------------------------------ Saga Saiyajin
    {
        id = "s1_raditz",
        saga = "Saga Saiyajin",
        titulo = "El hermano de Goku",
        pareja = "Goku (Inicio) y Piccoro (Inicio)",
        rivales = {
            { personaje = 77, nivel = 3, vida = 200, nombre = "Raditz" },
            { personaje = 184, nivel = 2, vida = 100, nombre = "Saibaman" },
        },
        refuerzos = {
            { personaje = 184, nivel = 2, vida = 100, nombre = "Saibaman" },
            { personaje = 184, nivel = 2, vida = 100, nombre = "Saibaman" },
        },
        escenario = -1, musica = -1, tiempo = 4,
        intro = "Un saiyajin llamado Raditz ha llegado a la Tierra y no viene solo: ha plantado semillas de "
             .. "Saibaman. Cuando caiga uno, brotará otro. Raditz tiene el doble de vida.",
        cierre = "Raditz ha caído. Antes de morir avisa: llegarán dos saiyajin mucho más fuertes.",
        derrota = "Raditz se ríe: «¡Basura!». Hay que entrenar más.",
    },
    {
        id = "s2_saibamen",
        saga = "Saga Saiyajin",
        titulo = "El jardín de los Saibaman",
        pareja = "Krilin y Tenshinhan",
        rivales = {
            { personaje = 184, nivel = 2, vida = 110, nombre = "Saibaman" },
            { personaje = 184, nivel = 2, vida = 110, nombre = "Saibaman" },
            { personaje = 184, nivel = 2, vida = 110, nombre = "Saibaman" },
        },
        refuerzos = {
            { personaje = 184, nivel = 2, vida = 110, nombre = "Saibaman" },
            { personaje = 184, nivel = 2, vida = 110, nombre = "Saibaman" },
            { personaje = 184, nivel = 2, vida = 110, nombre = "Saibaman" },
        },
        escenario = -1, musica = -1, tiempo = 4,
        intro = "Nappa planta seis Saibaman para entretenerse. Tres a la vez y tres más esperando. "
             .. "Cubríos las espaldas: si os separan, os rodean.",
        cierre = "No queda ni un Saibaman. Nappa deja de sonreír.",
        derrota = "Los Saibaman os han rodeado. Nappa aplaude.",
    },
    {
        id = "s3_elite",
        saga = "Saga Saiyajin",
        titulo = "La élite saiyajin",
        pareja = "Goku (Inicio) y Gohan (Niño)",
        rivales = {
            { personaje = 80, nivel = 2, vida = 200, nombre = "Nappa" },
            { personaje = 29, nivel = 3, vida = 160, nombre = "Vegeta (Explorador)" },
        },
        refuerzos = {
            { personaje = 170, nivel = 2, vida = 120, nombre = "Nappa (Ozaru)" },
            { personaje = 30, nivel = 3, vida = 180, nombre = "Vegeta (Ozaru)" },
        },
        escenario = -1, musica = -1, tiempo = 4,
        intro = "Nappa y Vegeta. Cuando creáis que los habéis vencido, mirarán a la luna... "
             .. "Guardad energía: lo peor viene después.",
        cierre = "Vegeta huye en su nave, herido en su orgullo. Jura que volverá.",
        derrota = "Vegeta: «Ni juntos sois rival para la élite saiyajin».",
    },
    -- ------------------------------------------------------------------ Saga Freezer
    {
        id = "f1_esbirros",
        saga = "Saga Freezer",
        titulo = "Los esbirros de Freezer",
        pareja = "Gohan (Niño) y Krilin",
        rivales = {
            { personaje = 84, nivel = 2, vida = 160, nombre = "Dodoria" },
            { personaje = 82, nivel = 2, vida = 150, nombre = "Zarbon" },
        },
        refuerzos = {
            { personaje = 79, nivel = 2, vida = 130, nombre = "Cui" },
            { personaje = 83, nivel = 3, vida = 170, nombre = "Zarbon (Transformado)" },
        },
        escenario = -1, musica = -1, tiempo = 4,
        intro = "En Namek, los hombres de Freezer buscan las Bolas de Dragón. Dodoria, Zarbon y Cui. "
             .. "Zarbon guarda una transformación para el final.",
        cierre = "Los esbirros han caído. Freezer tendrá que llamar a sus mejores hombres.",
        derrota = "Zarbon se arregla el pelo: «Qué poco elegante».",
    },
    {
        id = "f2_ginyu",
        saga = "Saga Freezer",
        titulo = "Las Fuerzas Especiales Ginyu",
        pareja = "Goku (Med) y Vegeta",
        rivales = {
            { personaje = 87, nivel = 2, vida = 180, nombre = "Recoome" },
            { personaje = 88, nivel = 2, vida = 150, nombre = "Burter" },
            { personaje = 89, nivel = 2, vida = 150, nombre = "Jeice" },
        },
        refuerzos = {
            { personaje = 90, nivel = 2, vida = 130, nombre = "Guldo", detras = 2 },
            { personaje = 86, nivel = 3, vida = 220, nombre = "Capitán Ginyu", detras = 1 },
        },
        escenario = -1, musica = -1, tiempo = 4,
        intro = "¡Pose de combate! Recoome, Burter y Jeice a la vez, y Guldo y el Capitán Ginyu esperando "
             .. "su turno. Cinco guerreros de élite contra dos.",
        cierre = "¡Las Fuerzas Especiales Ginyu han caído! Freezer ya no tiene a quién enviar.",
        derrota = "«¡Pose de victoria!» Las Fuerzas Ginyu os han derrotado.",
    },
    {
        id = "f3_formas",
        saga = "Saga Freezer",
        titulo = "Las formas de Freezer",
        pareja = "Piccoro (Fin) y Gohan (Niño)",
        rivales = {
            { personaje = 91, nivel = 3, vida = 200, nombre = "Freezer (1.ª forma)" },
        },
        refuerzos = {
            { personaje = 92, nivel = 3, vida = 220, nombre = "Freezer (2.ª forma)" },
            { personaje = 93, nivel = 3, vida = 240, nombre = "Freezer (3.ª forma)" },
            { personaje = 94, nivel = 3, vida = 260, nombre = "Freezer (Forma final)" },
        },
        escenario = -1, musica = -1, tiempo = 4,
        intro = "Freezer en persona. Uno contra dos parece fácil... hasta que se transforma. Y otra vez. "
             .. "Y otra. Cuatro formas, cada una más fuerte que la anterior.",
        cierre = "Freezer revela su forma final... y cae. Pero aún no ha mostrado todo su poder.",
        derrota = "Freezer: «Os mostraré el verdadero terror».",
    },
    {
        id = "f4_cien",
        saga = "Saga Freezer",
        titulo = "El 100% de su poder",
        pareja = "Goku (Med) y Piccoro (Fin)",
        rivales = {
            { personaje = 95, nivel = 3, vida = 300, nombre = "Freezer (100% de poder)" },
            { personaje = 97, nivel = 2, vida = 180, nombre = "King Cold" },
        },
        refuerzos = {
            { personaje = 96, nivel = 3, vida = 200, nombre = "Mecha Freezer" },
        },
        escenario = -1, musica = -1, tiempo = 4,
        intro = "Namek explota en cinco minutos y Freezer usa el 100% de su poder. Su padre, King Cold, "
             .. "ha venido a ayudarle. Y si cae... vuelve convertido en máquina.",
        cierre = "Freezer, su padre y su cuerpo mecánico han caído. El universo respira tranquilo.",
        derrota = "Freezer ríe mientras todo explota.",
    },
    -- ------------------------------------------------------------------ Saga de los Androides
    {
        id = "a1_gero",
        saga = "Saga de los Androides",
        titulo = "El laboratorio del Dr. Gero",
        pareja = "Piccoro (Fin) y Krilin",
        rivales = {
            { personaje = 103, nivel = 2, vida = 200, nombre = "Androide n.º 19" },
            { personaje = 185, nivel = 3, vida = 180, nombre = "Dr. Gero" },
        },
        refuerzos = {
            { personaje = 101, nivel = 2, vida = 150, nombre = "Androide n.º 17" },
            { personaje = 102, nivel = 2, vida = 150, nombre = "Androide n.º 18" },
        },
        escenario = -1, musica = -1, tiempo = 4,
        intro = "Los androides absorben energía: no les lancéis ráfagas sin pensar. Y si derrotáis al "
             .. "Dr. Gero, despertará a sus creaciones más peligrosas.",
        cierre = "El laboratorio está en ruinas. Pero algo se mueve en las alcantarillas...",
        derrota = "El Dr. Gero apunta vuestros datos para mejorar a sus androides.",
    },
    {
        id = "a2_trio",
        saga = "Saga de los Androides",
        titulo = "Los tres androides",
        pareja = "Trunks y Vegeta",
        rivales = {
            { personaje = 100, nivel = 2, vida = 220, nombre = "Androide n.º 16" },
            { personaje = 101, nivel = 2, vida = 160, nombre = "Androide n.º 17" },
            { personaje = 102, nivel = 2, vida = 160, nombre = "Androide n.º 18" },
        },
        refuerzos = {
            { personaje = 105, nivel = 3, vida = 220, nombre = "Cell (1.ª forma)" },
        },
        escenario = -1, musica = -1, tiempo = 4,
        intro = "16, 17 y 18 juntos, con energía infinita. Tres contra dos. Y mientras peleáis, "
             .. "alguien os observa desde el bosque, esperando su momento.",
        cierre = "Los androides se retiran... y Cell, sin conseguir absorberlos, se esconde.",
        derrota = "18 bosteza: «¿Ya está?».",
    },
    -- ------------------------------------------------------------------ Saga Cell
    {
        id = "c1_imperfecto",
        saga = "Saga Cell",
        titulo = "La evolución de Cell",
        pareja = "Gohan (Joven) y Piccoro (Fin)",
        rivales = {
            { personaje = 105, nivel = 3, vida = 220, nombre = "Cell (1.ª forma)" },
            { personaje = 100, nivel = 2, vida = 180, nombre = "Androide n.º 16" },
        },
        refuerzos = {
            { personaje = 106, nivel = 3, vida = 240, nombre = "Cell (2.ª forma)", detras = 1 },
            { personaje = 107, nivel = 3, vida = 260, nombre = "Cell (Perfecto)", detras = 1 },
        },
        escenario = -1, musica = -1, tiempo = 4,
        intro = "Cell ha absorbido a 17 y 16 se interpone en vuestro camino. Cada vez que Cell cae, "
             .. "vuelve más fuerte. Llegará a su forma perfecta.",
        cierre = "Cell, ya perfecto, anuncia un torneo. Tenéis diez días para prepararos.",
        derrota = "Cell: «¡Perfecto!».",
    },
    {
        id = "c2_hijos",
        saga = "Saga Cell",
        titulo = "Los hijos de Cell",
        pareja = "Gohan (Joven) y Vegeta",
        rivales = {
            { personaje = 109, nivel = 2, vida = 120, nombre = "Cell Jr." },
            { personaje = 109, nivel = 2, vida = 120, nombre = "Cell Jr." },
            { personaje = 109, nivel = 2, vida = 120, nombre = "Cell Jr." },
        },
        refuerzos = {
            { personaje = 109, nivel = 2, vida = 120, nombre = "Cell Jr." },
            { personaje = 109, nivel = 2, vida = 120, nombre = "Cell Jr." },
            { personaje = 109, nivel = 2, vida = 120, nombre = "Cell Jr." },
            { personaje = 109, nivel = 2, vida = 120, nombre = "Cell Jr." },
        },
        escenario = -1, musica = -1, tiempo = 4,
        intro = "Siete Cell Jr. Pequeños, rápidos y crueles. Tres a la vez y cuatro esperando. "
             .. "No os dejéis rodear.",
        cierre = "No queda ninguno. Cell se levanta de su asiento, furioso.",
        derrota = "Los Cell Jr. juegan con vosotros como su padre.",
    },
    {
        id = "c3_juegos",
        saga = "Saga Cell",
        titulo = "Los Juegos de Cell",
        pareja = "Gohan (Joven) y Goku (Fin)",
        rivales = {
            { personaje = 107, nivel = 3, vida = 300, nombre = "Cell (Super Perfecto)" },
            { personaje = 109, nivel = 2, vida = 120, nombre = "Cell Jr." },
        },
        refuerzos = {
            { personaje = 108, nivel = 3, vida = 250, nombre = "Cell Max", detras = 1 },
            { personaje = 109, nivel = 2, vida = 120, nombre = "Cell Jr.", detras = 2 },
        },
        escenario = -1, musica = -1, tiempo = 4,
        intro = "El jefe final de la saga: Cell Super Perfecto con el triple de vida, sus hijos, "
             .. "y algo enorme si conseguís tumbarlo. ¡Kamehameha padre e hijo!",
        cierre = "¡Cell ha sido derrotado! La Tierra está a salvo gracias a vuestro trabajo en equipo.",
        derrota = "Cell se ríe. La Tierra está perdida... de momento.",
    },
    -- ------------------------------------------------------------------ Saga de Bu
    {
        id = "b1_babidi",
        saga = "Saga de Bu",
        titulo = "La nave de Babidi",
        pareja = "Gohan y Vegeta (Majin)",
        rivales = {
            { personaje = 194, nivel = 2, vida = 150, nombre = "Spopovich" },
            { personaje = 111, nivel = 3, vida = 220, nombre = "Dabura" },
        },
        refuerzos = {
            { personaje = 112, nivel = 3, vida = 280, nombre = "Majin Bu" },
        },
        escenario = -1, musica = -1, tiempo = 4,
        intro = "Babidi quiere despertar a Majin Bu con vuestra energía. Spopovich y Dabura, rey de los "
             .. "demonios, os cortan el paso. Si tardáis demasiado... Bu despierta.",
        cierre = "Bu está fuera de control, pero de momento se ha ido a buscar dulces.",
        derrota = "Babidi: «¡Papparapa!». Bu ha despertado.",
    },
    {
        id = "b2_superbu",
        saga = "Saga de Bu",
        titulo = "El terror de Súper Bu",
        pareja = "Gotenks y Gohan Definitivo",
        rivales = {
            { personaje = 114, nivel = 3, vida = 250, nombre = "Súper Bu" },
            { personaje = 112, nivel = 2, vida = 160, nombre = "Majin Bu" },
        },
        refuerzos = {
            { personaje = 115, nivel = 3, vida = 260, nombre = "Súper Bu (Gotenks absorbido)", detras = 1 },
            { personaje = 116, nivel = 3, vida = 300, nombre = "Súper Bu (Gohan absorbido)", detras = 1 },
        },
        escenario = -1, musica = -1, tiempo = 4,
        intro = "Súper Bu absorbe a los guerreros más fuertes. Cada vez que cae, vuelve con el poder de "
             .. "otro. El penúltimo reto.",
        cierre = "Súper Bu se tambalea... y de su interior sale algo más pequeño y mucho más peligroso.",
        derrota = "Bu os convierte en chocolate.",
    },
    {
        id = "b3_puro",
        saga = "Saga de Bu",
        titulo = "La forma pura",
        pareja = "Goku (Fin) y Vegeta",
        rivales = {
            { personaje = 117, nivel = 3, vida = 350, nombre = "Bu (Forma pura)" },
            { personaje = 114, nivel = 3, vida = 200, nombre = "Súper Bu" },
        },
        refuerzos = {
            { personaje = 192, nivel = 3, vida = 220, nombre = "Majin Bu (Pura maldad)", detras = 2 },
            { personaje = 117, nivel = 3, vida = 250, nombre = "Bu (Forma pura)", detras = 1 },
        },
        escenario = -1, musica = -1, tiempo = 4,
        intro = "El final. Bu en su forma pura, sin control ni piedad, y todo lo que ha sido antes. "
             .. "Si superáis esto, sois los guerreros más fuertes del universo.",
        cierre = "¡Bu ha sido derrotado! Habéis completado la Historia cooperativa. ¡Enhorabuena!",
        derrota = "Bu Puro grita y el planeta tiembla. Otra vez.",
    },
}

-- Números de personaje (lista sacada del propio juego):
--   0 Goku (Inicio)  1 Goku (Med)  3 Goku (Fin)  13 Gohan (Niño)  14 Gohan (Joven)  17 Gohan  21 Gohan Definitivo
--   22 Piccoro (Inicio)  23 Piccoro (Fin)  25 Krilin  26 Yamcha  27 Tenshinhan  29 Vegeta (Explorador)
--   30 Vegeta (Explorador, Ozaru)  31 Vegeta  37 Vegeta (Majin)  41 Trunks  48 Gotenks  51 Vegetto  73 Bardock
--   77 Raditz  79 Cui  80 Nappa  82 Zarbon  83 Zarbon (Transformado)  84 Dodoria  86 Ginyu  87 Recoome  88 Burter
--   89 Jeice  90 Guldo  91..95 Freezer (1.ª forma .. 100% de poder)  96 Mecha Freezer  97 King Cold
--   100..103 Androides 16..19  105 Cell (1.ª forma)  106 Cell (2.ª forma)  107 Cell (Super Perfecto)  108 Cell Max
--   109 Cell Jr.  111 Dabura  112 Majin Bu  114 Súper Bu  115 Súper Bu (Gotenks)  116 Súper Bu (Gohan)
--   117 Bu (Forma pura)  121 Turles  123 Slug  126 Cooler  127 Cooler (Forma final)  128 Metal Cooler
--   137 Janemba  166 Bojack  170 Nappa (Ozaru)  184 Saibaman  185 Dr. Gero  188 Broly  190 Broly (Legendario)
--   192 Majin Bu (Pura maldad)  194 Spopovich
