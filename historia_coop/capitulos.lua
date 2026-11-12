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
--   requiere   (opcional) ruta alternativa: se abre al completar todos los capítulos de esa saga
--   semanal    (opcional) true: jefe semanal; cada semana se abre uno de ellos (el mismo para todos)
--   secreto    (opcional) capítulo secreto: se abre al sacar nota S o Z en todos los capítulos de esa saga
--   dialogo    (opcional) la conversación antes del combate, línea a línea: { {"Quién", "Qué dice"}, ... }
--              (hasta 8 líneas)
--
-- En un rival o refuerzo:
--   transforma (opcional) el % de vida al que se transforma: cuando baja de ahí, entra el siguiente refuerzo
--              que tiene detrás (su forma siguiente) en vez de esperar a que caiga del todo
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
        dialogo = {
            { "Raditz", "Kakarotto, hermano. Has olvidado quién eres: un saiyajin enviado a conquistar este planeta." },
            { "Goku", "¡Yo me llamo Goku y la Tierra es mi hogar!" },
            { "Piccoro", "Por esta vez lucharé a tu lado. No te acostumbres." },
        },
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
        dialogo = {
            { "Nappa", "Vamos a divertirnos un rato. ¡Plantad las semillas!" },
            { "Krilin", "¿Esas cosas verdes salen de la tierra? Qué asco..." },
            { "Tenshinhan", "No bajes la guardia. Son más fuertes de lo que parecen." },
        },
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
        dialogo = {
            { "Vegeta", "Habéis llegado lejos, insectos. Aquí termina vuestro viaje." },
            { "Gohan", "Papá, tengo miedo... pero no voy a huir." },
            { "Goku", "Tranquilo, Gohan. Juntos podemos con ellos." },
        },
        rivales = {
            { personaje = 80, nivel = 2, vida = 200, nombre = "Nappa" },
            { personaje = 29, nivel = 3, vida = 160, nombre = "Vegeta (Explorador)" },
        },
        refuerzos = {
            { personaje = 80, nivel = 3, vida = 150, nombre = "Nappa (furioso)" },
            { personaje = 29, nivel = 3, vida = 200, nombre = "Vegeta (al límite)" },
        },
        escenario = -1, musica = -1, tiempo = 4,
        intro = "Nappa y Vegeta. Cuando creáis que los habéis vencido, volverán a levantarse "
             .. "más furiosos. Guardad energía: lo peor viene después.",
        cierre = "Vegeta huye en su nave, herido en su orgullo. Jura que volverá.",
        derrota = "Vegeta: «Ni juntos sois rival para la élite saiyajin».",
    },
    -- ------------------------------------------------------------------ Saga Freezer
    {
        id = "f1_esbirros",
        saga = "Saga Freezer",
        titulo = "Los esbirros de Freezer",
        pareja = "Gohan (Niño) y Krilin",
        dialogo = {
            { "Zarbon", "Freezer quiere las Bolas de Dragón. Vosotros sobráis." },
            { "Krilin", "Gohan, el radar dice que hay más ahí fuera. Vienen muchos." },
            { "Gohan", "¡Pues que vengan! No dejaremos que Freezer pida su deseo." },
        },
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
        dialogo = {
            { "Capitán Ginyu", "¡Fuerzas Especiales Ginyu, posición de combate!" },
            { "Vegeta", "Kakarotto, no te metas en mi camino. A estos payasos los conozco." },
            { "Goku", "Son cinco. Nosotros, dos. ¡Me gusta!" },
        },
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
        dialogo = {
            { "Freezer", "¿Sabéis cuántas veces puedo transformarme? Os lo voy a enseñar... una a una." },
            { "Piccoro", "Cada forma será más fuerte. Guarda fuerzas, Gohan." },
            { "Gohan", "Si cae una, vendrá la siguiente. ¡Estamos listos!" },
        },
        rivales = {
            { personaje = 91, nivel = 3, vida = 200, transforma = 40, nombre = "Freezer (1.ª forma)" },
        },
        refuerzos = {
            { personaje = 92, nivel = 3, vida = 220, transforma = 40, nombre = "Freezer (2.ª forma)" },
            { personaje = 93, nivel = 3, vida = 240, transforma = 40, nombre = "Freezer (3.ª forma)" },
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
        dialogo = {
            { "Freezer", "Esta vez usaré el cien por cien de mi poder. Namek tiene los minutos contados." },
            { "King Cold", "Hijo, déjame ayudarte. Acabemos con estos monos." },
            { "Goku", "Piccoro, ¿listo? Esto termina hoy." },
        },
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
        dialogo = {
            { "Dr. Gero", "Mis creaciones son perfectas. Vosotros sois solo datos." },
            { "Krilin", "¡Ese viejo se ha convertido en un androide a sí mismo!" },
            { "Piccoro", "Hay que encontrar su laboratorio antes de que despierte a los demás." },
        },
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
        dialogo = {
            { "Androide n.º 17", "Teníamos un objetivo, pero esto es más divertido." },
            { "Trunks", "En mi futuro estos androides lo destruyeron todo." },
            { "Vegeta", "Pues aquí no estamos en tu futuro. ¡Apártate!" },
        },
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
        dialogo = {
            { "Cell", "Necesito a los androides para alcanzar la perfección. No os metáis." },
            { "Androide n.º 16", "No le dejaré absorber a nadie más." },
            { "Gohan", "Cada vez que lo derrotamos vuelve más fuerte. ¡Hay que pararlo ya!" },
        },
        rivales = {
            { personaje = 105, nivel = 3, vida = 220, transforma = 40, nombre = "Cell (1.ª forma)" },
            { personaje = 100, nivel = 2, vida = 180, nombre = "Androide n.º 16" },
        },
        refuerzos = {
            { personaje = 106, nivel = 3, vida = 240, transforma = 40, nombre = "Cell (2.ª forma)", detras = 1 },
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
        dialogo = {
            { "Cell", "Os presento a mis hijos. Jugad con ellos." },
            { "Vegeta", "¿Siete pequeños monstruos? Hasta me ofendes." },
            { "Gohan", "No os separéis. Si nos rodean, estamos perdidos." },
        },
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
        dialogo = {
            { "Cell", "Bienvenidos a mis juegos. El ganador decide el destino de la Tierra." },
            { "Goku", "Gohan, sé que llevas un gran poder dentro. Es tu momento." },
            { "Gohan", "No dejaré que sigas haciendo daño, Cell." },
        },
        rivales = {
            { personaje = 107, nivel = 3, vida = 300, nombre = "Cell (Super Perfecto)" },
            { personaje = 109, nivel = 2, vida = 120, nombre = "Cell Jr." },
        },
        refuerzos = {
            { personaje = 107, nivel = 3, vida = 200, nombre = "Cell (regenerado)", detras = 1 },
            { personaje = 109, nivel = 2, vida = 120, nombre = "Cell Jr.", detras = 2 },
        },
        escenario = -1, musica = -1, tiempo = 4,
        intro = "El jefe final de la saga: Cell Super Perfecto con el triple de vida y sus hijos. "
             .. "Y si conseguís tumbarlo... se regenera. ¡Kamehameha padre e hijo!",
        cierre = "¡Cell ha sido derrotado! La Tierra está a salvo gracias a vuestro trabajo en equipo.",
        derrota = "Cell se ríe. La Tierra está perdida... de momento.",
    },
    -- ------------------------------------------------------------------ Saga de Bu
    {
        id = "b1_babidi",
        saga = "Saga de Bu",
        titulo = "La nave de Babidi",
        pareja = "Gohan y Vegeta (Majin)",
        dialogo = {
            { "Babidi", "¡Despertad a Majin Bu! ¡Necesito toda la energía!" },
            { "Vegeta", "Babidi me controla... pero mi orgullo es mío." },
            { "Gohan", "Tenemos que parar esto antes de que el monstruo despierte." },
        },
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
        dialogo = {
            { "Súper Bu", "Ahora soy más fuerte. Y tengo hambre." },
            { "Gotenks", "¡El Súper Gotenks ha llegado! Prepárate a perder." },
            { "Gohan", "Ten cuidado: si absorbe a alguien, cambia de forma." },
        },
        rivales = {
            { personaje = 114, nivel = 3, vida = 250, transforma = 40, nombre = "Súper Bu" },
            { personaje = 112, nivel = 2, vida = 160, nombre = "Majin Bu" },
        },
        refuerzos = {
            { personaje = 115, nivel = 3, vida = 260, transforma = 40, nombre = "Súper Bu (Gotenks absorbido)", detras = 1 },
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
        dialogo = {
            { "Bu", "¡Ñaaaa!" },
            { "Vegeta", "Kakarotto, esta vez lucharemos juntos. Solo esta vez." },
            { "Goku", "Es el último combate. ¡Dalo todo, Vegeta!" },
        },
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
    -- ------------------------------------------------------------------ Saga de las Películas
    {
        id = "p1_turles",
        saga = "Saga de las Películas",
        titulo = "El Árbol del Poder",
        pareja = "Goku (Med) y Gohan (Niño)",
        dialogo = {
            { "Turles", "El Árbol del Poder se beberá la energía de la Tierra. Y yo comeré su fruto." },
            { "Goku", "¡Te pareces a mí! Pero no tienes nada que ver conmigo." },
            { "Gohan", "¡Papá, el árbol está secando el planeta!" },
        },
        rivales = {
            { personaje = 121, nivel = 3, vida = 220, nombre = "Turles" },
            { personaje = 184, nivel = 2, vida = 110, nombre = "Saibaman" },
            { personaje = 184, nivel = 2, vida = 110, nombre = "Saibaman" },
        },
        refuerzos = {
            { personaje = 184, nivel = 2, vida = 110, nombre = "Saibaman", detras = 2 },
            { personaje = 184, nivel = 2, vida = 110, nombre = "Saibaman", detras = 3 },
            { personaje = 121, nivel = 3, vida = 160, nombre = "Turles (fruto del árbol)", detras = 1 },
        },
        escenario = -1, musica = -1, tiempo = 4,
        intro = "Turles, un saiyajin idéntico a Goku, ha plantado el Árbol del Poder y se come sus frutos. "
             .. "Cuando creáis que ha caído, comerá otro fruto y volverá con fuerzas renovadas.",
        cierre = "El Árbol del Poder se derrumba y Turles con él. La Tierra vuelve a respirar.",
        derrota = "Turles se come otro fruto mientras os mira desde arriba.",
    },
    {
        id = "p2_slug",
        saga = "Saga de las Películas",
        titulo = "El súper namekiano",
        pareja = "Piccoro (Fin) y Gohan (Niño)",
        dialogo = {
            { "Slug", "Este planeta será mi nave. Pronto será de noche para siempre." },
            { "Piccoro", "Un namekiano usando su poder para el mal... qué vergüenza." },
            { "Gohan", "¡Señor Piccoro, yo le cubro!" },
        },
        rivales = {
            { personaje = 123, nivel = 3, vida = 260, nombre = "Slug" },
            { personaje = 182, nivel = 2, vida = 150, nombre = "Garlick Jr." },
        },
        refuerzos = {
            { personaje = 183, nivel = 3, vida = 200, nombre = "Súper Garlick Jr.", detras = 2 },
        },
        escenario = -1, musica = -1, tiempo = 4,
        intro = "Slug, un namekiano malvado, quiere congelar la Tierra para convertirla en su nave. "
             .. "Garlick Jr. ha vuelto de la Zona Muerta para ayudarle. Es la hora de Piccoro.",
        cierre = "Slug y Garlick Jr. han caído. Piccoro sonríe, aunque no lo reconocerá nunca.",
        derrota = "La Tierra se queda a oscuras bajo el dominio de Slug.",
    },
    {
        id = "p3_cooler",
        saga = "Saga de las Películas",
        titulo = "La venganza de Cooler",
        pareja = "Goku (Med) y Piccoro (Fin)",
        dialogo = {
            { "Cooler", "Mi hermano era un descuidado. Yo no cometo errores." },
            { "Piccoro", "Otro de esa familia. Esto no se acaba nunca." },
            { "Goku", "¡Pues lo acabamos ahora!" },
        },
        rivales = {
            { personaje = 126, nivel = 3, vida = 220, nombre = "Cooler" },
            { personaje = 186, nivel = 2, vida = 160, nombre = "Salza" },
        },
        refuerzos = {
            { personaje = 127, nivel = 3, vida = 280, nombre = "Cooler (Forma final)", detras = 1 },
            { personaje = 128, nivel = 3, vida = 200, nombre = "Metal Cooler", detras = 2 },
        },
        escenario = -1, musica = -1, tiempo = 4,
        intro = "El hermano mayor de Freezer viene a vengar a la familia, con su escuadrón personal. "
             .. "Cooler tiene una transformación que su hermano nunca tuvo. Y no será la última vez que lo veáis.",
        cierre = "Cooler cae al sol. Pero en algún lugar del espacio, una máquina empieza a reconstruirlo...",
        derrota = "Cooler: «Mi hermano era un aficionado».",
    },
    {
        id = "p4_bojack",
        saga = "Saga de las Películas",
        titulo = "El torneo de Bojack",
        pareja = "Gohan (Joven) y Trunks",
        dialogo = {
            { "Bojack", "¿Este torneo? Será vuestro funeral." },
            { "Trunks", "Gohan, son piratas espaciales. Muy fuertes." },
            { "Gohan", "Mi padre ya no está. Ahora protegemos nosotros la Tierra." },
        },
        rivales = {
            { personaje = 166, nivel = 3, vida = 220, nombre = "Bojack" },
            { personaje = 187, nivel = 2, vida = 160, nombre = "Zangya" },
        },
        refuerzos = {
            { personaje = 136, nivel = 3, vida = 260, nombre = "Súper Bojack", detras = 1 },
            { personaje = 187, nivel = 2, vida = 140, nombre = "Zangya", detras = 2 },
        },
        escenario = -1, musica = -1, tiempo = 4,
        intro = "Bojack y su banda de piratas espaciales han interrumpido el torneo de Mr. Satan. "
             .. "Goku no está: el mundo depende de los jóvenes guerreros.",
        cierre = "Bojack ha caído. Gohan ha demostrado que es digno sucesor de su padre.",
        derrota = "Bojack ríe: «¿Esto es lo mejor que tiene la Tierra?».",
    },
    {
        id = "p5_broly",
        saga = "Saga de las Películas",
        titulo = "El Súper Saiyajin Legendario",
        pareja = "Goku (Fin) y Vegeta",
        dialogo = {
            { "Broly", "¡Kakarottooo!" },
            { "Vegeta", "Su poder no tiene fin... es el Súper Saiyajin Legendario." },
            { "Goku", "Vegeta, sin ti no podré. ¡Ayúdame!" },
        },
        rivales = {
            { personaje = 188, nivel = 3, vida = 220, nombre = "Broly" },
        },
        refuerzos = {
            { personaje = 189, nivel = 3, vida = 260, nombre = "Broly (Súper Saiyajin)" },
            { personaje = 190, nivel = 4, vida = 350, nombre = "Broly (Legendario)" },
        },
        escenario = -1, musica = -1, tiempo = 4,
        intro = "Un saiyajin nacido con un poder sin límites. Cada vez que lo derribáis se levanta más fuerte "
             .. "y más furioso. Solo dos contra uno, pero ese uno es Broly.",
        cierre = "Broly cae por fin. Vegeta tiembla un poco, pero dirá que era el frío.",
        derrota = "«¡KAKAROTTO!» Broly no ha tenido piedad.",
    },
    {
        id = "p6_janemba",
        saga = "Saga de las Películas",
        titulo = "El caos del Más Allá",
        pareja = "Goku (Fin) y Vegeta",
        dialogo = {
            { "Janemba", "(Se ríe y el Más Allá se retuerce a su alrededor.)" },
            { "Vegeta", "Los muertos vuelven a la vida por culpa de este monstruo." },
            { "Goku", "Si no lo paramos, se mezclarán los dos mundos." },
        },
        rivales = {
            { personaje = 137, nivel = 3, vida = 280, nombre = "Janemba" },
            { personaje = 97, nivel = 2, vida = 160, nombre = "King Cold (resucitado)" },
            { personaje = 94, nivel = 2, vida = 160, nombre = "Freezer (resucitado)" },
        },
        refuerzos = {
            { personaje = 138, nivel = 4, vida = 320, nombre = "Súper Janemba", detras = 1 },
            { personaje = 107, nivel = 2, vida = 160, nombre = "Cell (resucitado)", detras = 3 },
        },
        escenario = -1, musica = -1, tiempo = 4,
        intro = "Janemba ha roto la frontera entre la vida y la muerte, y los villanos del pasado han vuelto. "
             .. "Freezer, King Cold y Cell os esperan. Janemba guarda su verdadera forma para el final.",
        cierre = "Janemba ha sido purificado. Los muertos vuelven a su sitio... por ahora.",
        derrota = "El Más Allá se queda sumido en el caos.",
    },
    -- ------------------------------------------------------------------ Saga GT
    {
        id = "g1_baby",
        saga = "Saga GT",
        titulo = "El parásito Baby",
        pareja = "Goku (GT) y Pan",
        dialogo = {
            { "Baby", "Los saiyajin destruyeron a mi pueblo. Ahora todos me obedeceréis." },
            { "Pan", "¡Abuelo, ha controlado a todos los de la Tierra!" },
            { "Goku", "Pan, quédate cerca. Vamos a sacarlo de ahí." },
        },
        rivales = {
            { personaje = 140, nivel = 3, vida = 240, nombre = "Baby Vegeta" },
            { personaje = 31, nivel = 2, vida = 150, nombre = "Vegeta (controlado)" },
        },
        refuerzos = {
            { personaje = 141, nivel = 3, vida = 280, nombre = "Súper Baby", detras = 1 },
            { personaje = 142, nivel = 4, vida = 300, nombre = "Súper Baby (2.ª forma)", detras = 1 },
        },
        escenario = -1, musica = -1, tiempo = 4,
        intro = "Baby, el último tsufur, ha poseído a Vegeta y a casi toda la Tierra. "
             .. "Cada vez que lo derrotáis, crece y se hace más fuerte.",
        cierre = "Baby abandona el cuerpo de Vegeta y huye. La Tierra es libre otra vez.",
        derrota = "Baby: «Todos los saiyajin seréis mis esclavos».",
    },
    {
        id = "g2_17",
        saga = "Saga GT",
        titulo = "Súper Androide 17",
        pareja = "Goku (GT) y Androide n.º 18",
        dialogo = {
            { "Súper 17", "Dos androides se han unido. Soy más fuerte que nunca." },
            { "Androide n.º 18", "Ese ya no es mi hermano. Lo sacaré de ahí aunque tenga que golpearle." },
            { "Goku", "Te ayudo, 18. ¡A por él!" },
        },
        rivales = {
            { personaje = 101, nivel = 3, vida = 200, nombre = "Androide n.º 17 (Infierno)" },
            { personaje = 185, nivel = 2, vida = 150, nombre = "Dr. Gero" },
        },
        refuerzos = {
            { personaje = 144, nivel = 4, vida = 320, nombre = "Súper Androide 17", detras = 1 },
            { personaje = 103, nivel = 2, vida = 150, nombre = "Androide n.º 19", detras = 2 },
        },
        escenario = -1, musica = -1, tiempo = 4,
        intro = "El Dr. Gero ha unido a los dos Androides 17, el de la Tierra y el del Infierno. "
             .. "El resultado absorbe la energía de todo lo que le lanzáis.",
        cierre = "Súper 17 estalla. 18 ha vengado a Krilin.",
        derrota = "Súper 17 absorbe vuestra última ráfaga y sonríe.",
    },
    {
        id = "g3_shenron",
        saga = "Saga GT",
        titulo = "Los Dragones Oscuros",
        pareja = "Goku (GT) y Vegeta (GT)",
        dialogo = {
            { "Omega Shenron", "Las Bolas de Dragón se han cargado de energía negativa. Soy el resultado." },
            { "Vegeta", "Los dragones ya no conceden deseos. Ahora toca pelear con ellos." },
            { "Goku", "Esta vez no hay deseo que nos salve. Solo nosotros." },
        },
        rivales = {
            { personaje = 146, nivel = 3, vida = 220, nombre = "Eis Shenron" },
            { personaje = 145, nivel = 3, vida = 220, nombre = "Neo Shenron" },
        },
        refuerzos = {
            { personaje = 197, nivel = 3, vida = 280, nombre = "Syn Shenron", detras = 1 },
            { personaje = 147, nivel = 4, vida = 380, nombre = "Omega Shenron", detras = 2 },
        },
        escenario = -1, musica = -1, tiempo = 4,
        intro = "Usar las Bolas de Dragón tantas veces tenía un precio: los Dragones Oscuros. "
             .. "El último de ellos ha absorbido las siete esferas. El final de todo Dragon Ball.",
        cierre = "Omega Shenron desaparece en una Genkidama. Gracias por jugar, guerreros.",
        derrota = "Omega Shenron: «Este universo es mío».",
    },
    -- ------------------------------------------------------------------ Saga Super
    {
        id = "u1_golden",
        saga = "Saga Super",
        titulo = "La resurrección de Freezer",
        pareja = "Goku (Fin) y Vegeta",
        dialogo = {
            { "Freezer", "He entrenado, monos. Admirad mi forma dorada." },
            { "Vegeta", "¿Freezer entrenando? El mundo se ha vuelto loco." },
            { "Goku", "Pues habrá que ver quién ha entrenado más." },
        },
        rivales = {
            { personaje = 94, nivel = 3, vida = 220, nombre = "Freezer (Forma final)" },
            { personaje = 79, nivel = 2, vida = 140, nombre = "Soldado de Freezer" },
            { personaje = 84, nivel = 2, vida = 140, nombre = "Soldado de Freezer" },
        },
        refuerzos = {
            { personaje = 70, nivel = 4, vida = 320, nombre = "Golden Freezer", detras = 1 },
            { personaje = 82, nivel = 2, vida = 140, nombre = "Soldado de Freezer", detras = 2 },
            { personaje = 88, nivel = 2, vida = 140, nombre = "Soldado de Freezer", detras = 3 },
        },
        escenario = -1, musica = -1, tiempo = 4,
        intro = "Freezer ha resucitado con mil soldados y cuatro meses de entrenamiento. "
             .. "Primero su ejército, después su nueva forma dorada.",
        cierre = "Golden Freezer cae. Esta vez el emperador ha aprendido a entrenar... y aun así no basta.",
        derrota = "Freezer: «¡Mirad mi forma más hermosa!».",
    },
    {
        id = "u2_black",
        saga = "Saga Super",
        titulo = "El futuro de Trunks",
        pareja = "Trunks (DBS) y Vegeta",
        dialogo = {
            { "Goku Black", "Los mortales sois un error. Yo corregiré el mundo." },
            { "Trunks", "En mi futuro ya no queda nada... por su culpa." },
            { "Vegeta", "Tiene la cara de Kakarotto. Más ganas tengo de pegarle." },
        },
        rivales = {
            { personaje = 99, nivel = 3, vida = 240, nombre = "Black Goku" },
            { personaje = 157, nivel = 3, vida = 220, nombre = "Zamas" },
        },
        refuerzos = {
            { personaje = 34, nivel = 3, vida = 260, nombre = "Black Goku (Rosé)", detras = 1 },
            { personaje = 76, nivel = 4, vida = 360, nombre = "Zamas (Fusión)", detras = 2 },
        },
        escenario = -1, musica = -1, tiempo = 4,
        intro = "En el futuro de Trunks, un dios ha robado el cuerpo de Goku. Black y Zamas son inmortales "
             .. "por separado... y cuando se fusionan, mucho peor.",
        cierre = "Zamas ha sido borrado. El futuro tiene, al menos, una oportunidad.",
        derrota = "Zamas: «Los mortales sois un error que hay que corregir».",
    },
    {
        id = "u3_u6",
        saga = "Saga Super",
        titulo = "Los saiyajin del Universo 6",
        pareja = "Goku (Fin) y Gohan",
        dialogo = {
            { "Cabba", "En el Universo 6 también hay saiyajin. ¡Enséñame vuestra fuerza!" },
            { "Gohan", "Son saiyajin de otro universo... y nada malos." },
            { "Goku", "¡Qué emoción! Vamos a ver de qué son capaces." },
        },
        rivales = {
            { personaje = 119, nivel = 3, vida = 180, nombre = "Caulifla (Súper Saiyajin)" },
            { personaje = 199, nivel = 3, vida = 200, nombre = "Kale (Súper Saiyajin)" },
            { personaje = 134, nivel = 2, vida = 160, nombre = "Cabba (Súper Saiyajin)" },
        },
        refuerzos = {
            { personaje = 104, nivel = 3, vida = 260, nombre = "Kefla", detras = 1 },
            { personaje = 154, nivel = 4, vida = 320, nombre = "Kefla (Súper Saiyajin)", detras = 1 },
            { personaje = 78, nivel = 3, vida = 220, nombre = "Hit", detras = 3 },
        },
        escenario = -1, musica = -1, tiempo = 4,
        intro = "Los saiyajin del Universo 6 quieren demostrar quién es más fuerte. Tres a la vez, y si Caulifla "
             .. "y Kale se ponen los pendientes Potara... Kefla. Y Hit espera su turno.",
        cierre = "Los saiyajin del Universo 6 os dan la mano. Ha sido un honor.",
        derrota = "Caulifla: «¡Así se lucha, hermana!».",
    },
    {
        id = "u4_poder",
        saga = "Saga Super",
        titulo = "El Torneo del Poder",
        pareja = "Goku (Fin) y Vegeta",
        dialogo = {
            { "Jiren", "La fuerza lo es todo. Lo demás no importa." },
            { "Vegeta", "Ese tipo no ha mostrado ni la mitad de su poder." },
            { "Goku", "Si caemos, desaparece nuestro universo. ¡No podemos perder!" },
        },
        rivales = {
            { personaje = 81, nivel = 3, vida = 240, nombre = "Toppo" },
            { personaje = 151, nivel = 4, vida = 380, nombre = "Jiren" },
        },
        refuerzos = {
            { personaje = 150, nivel = 4, vida = 300, nombre = "Toppo (Hakaishin)", detras = 1 },
            { personaje = 151, nivel = 4, vida = 300, nombre = "Jiren (al límite)", detras = 2 },
        },
        escenario = -1, musica = -1, tiempo = 4,
        intro = "El reto final. El Universo 11 manda a sus mejores: Toppo, aspirante a dios de la destrucción, "
             .. "y Jiren, el guerrero más fuerte que habéis visto. Jiren tiene el doble de vida... dos veces.",
        cierre = "¡Habéis ganado el Torneo del Poder! Ya no queda nadie en el universo que pueda con vosotros.",
        derrota = "Jiren os mira sin inmutarse: «La fuerza lo es todo».",
    },
    -- ------------------------------------------------------------------ Saga de los Gigantes (2 contra 1)
    {
        id = "x1_ozaru",
        saga = "Saga de los Gigantes",
        titulo = "La luna llena",
        pareja = "Krilin y Piccoro (Inicio)",
        dialogo = {
            { "Krilin", "¡No miréis la luna! ¡Se ha convertido en un mono gigante!" },
            { "Piccoro", "Hay que cortarle la cola o derribarlo. Lo que llegue antes." },
        },
        rivales = {
            { personaje = 12, nivel = 3, vida = 320, nombre = "Ozaru" },
        },
        escenario = -1, musica = -1, tiempo = 4,
        intro = "Ha salido la luna llena y un saiyajin se ha transformado en un mono gigante. Es lento pero cada golpe duele. Dos contra uno: rodeadlo.",
        cierre = "El Ozaru cae con un estruendo. Alguien debería cortarle la cola de una vez.",
        derrota = "El Ozaru ruge a la luna. Habéis acabado aplastados.",
    },
    {
        id = "x2_slug",
        saga = "Saga de los Gigantes",
        titulo = "Slug gigante",
        pareja = "Goku (Med) y Piccoro (Fin)",
        dialogo = {
            { "Slug", "Ahora soy tan grande como una montaña. ¡Aplastaos solos!" },
            { "Goku", "Grande, pero lento. ¡Vamos por los dos lados!" },
        },
        rivales = {
            { personaje = 124, nivel = 3, vida = 340, nombre = "Slug (Gigante)" },
        },
        escenario = -1, musica = -1, tiempo = 4,
        intro = "Slug ha crecido hasta el tamaño de una montaña. Atacad desde los lados: de frente no hay quien lo pare.",
        cierre = "El gigante se desploma. Ni el tamaño salva a Slug.",
        derrota = "Slug os aplasta con una sola mano.",
    },
    {
        id = "x3_cellmax",
        saga = "Saga de los Gigantes",
        titulo = "Cell Max",
        pareja = "Gohan (Joven) y Piccoro (Fin)",
        dialogo = {
            { "Gohan", "Ese Cell no piensa: solo destruye. Y es enorme." },
            { "Piccoro", "Atacad su cabeza. Es su punto débil." },
        },
        rivales = {
            { personaje = 108, nivel = 4, vida = 380, nombre = "Cell Max" },
        },
        escenario = -1, musica = -1, tiempo = 4,
        intro = "El Dr. Gero tenía un plan B: Cell Max, un monstruo gigante sin cerebro y sin piedad. Buscad su punto débil en la cabeza.",
        cierre = "Cell Max explota. El último invento del Dr. Gero ha fracasado.",
        derrota = "Cell Max arrasa con todo lo que encuentra.",
    },
    {
        id = "x4_hildegarn",
        saga = "Saga de los Gigantes",
        titulo = "El demonio de la flauta",
        pareja = "Goku (Fin) y Gohan",
        dialogo = {
            { "Gohan", "Esa flauta despertó a un demonio dormido." },
            { "Goku", "Cuanto más tardemos, más fuerte se hará. ¡Rápido!" },
        },
        rivales = {
            { personaje = 195, nivel = 4, vida = 380, nombre = "Hildegarn" },
        },
        escenario = -1, musica = -1, tiempo = 4,
        intro = "Hildegarn, el demonio que dormía en una melodía, ha despertado. Solo la flauta de Tapion lo debilita... y no la tenéis.",
        cierre = "Hildegarn vuelve a dormir para siempre. La melodía se apaga.",
        derrota = "La melodía de Hildegarn os deja sin fuerzas.",
    },
    {
        id = "x5_babyozaru",
        saga = "Saga de los Gigantes",
        titulo = "El Ozaru dorado",
        pareja = "Goku (GT) y Vegeta (GT)",
        dialogo = {
            { "Baby", "¡Ozaru dorado! ¡Nadie puede pararme ahora!" },
            { "Vegeta", "Kakarotto, una vez fui un mono así. Sé cómo pararlo." },
            { "Goku", "Pues guíanos, Vegeta. ¡Vamos!" },
        },
        rivales = {
            { personaje = 143, nivel = 4, vida = 400, nombre = "Súper Baby (Ozaru)" },
        },
        escenario = -1, musica = -1, tiempo = 4,
        intro = "Baby ha usado la luz de la Tierra para transformarse en un Ozaru dorado. El gigante más fuerte de todos: el último reto.",
        cierre = "El Ozaru dorado cae. Ya no queda ningún gigante en pie.",
        derrota = "El Ozaru dorado os mira desde lo alto y se ríe.",
    },
    -- ------------------------------------------------------------------ Capítulos secretos
    -- Cada uno se abre al sacar nota S o Z en todos los capítulos de su saga («secreto»).
    {
        id = "z1_elite",
        saga = "Capítulos secretos",
        secreto = "Saga Saiyajin",
        titulo = "La élite al completo",
        pareja = "Goku (Med) y Piccoro (Fin)",
        dialogo = {
            { "Vegeta", "¿Creíais que me había rendido? Esta vez no estoy solo." },
            { "Nappa", "¡Por fin un combate de verdad!" },
            { "Goku", "Os habéis hecho más fuertes... nosotros también." },
        },
        rivales = {
            { personaje = 80, nivel = 4, vida = 280, nombre = "Nappa" },
            { personaje = 29, nivel = 4, vida = 300, nombre = "Vegeta (Explorador)" },
        },
        refuerzos = {
            { personaje = 77, nivel = 4, vida = 220, nombre = "Raditz", detras = 1 },
            { personaje = 31, nivel = 4, vida = 260, nombre = "Vegeta", detras = 2 },
        },
        escenario = -1, musica = -1, tiempo = 4,
        intro = "Capítulo secreto. Los saiyajin vuelven con todo: Nappa, Vegeta y Raditz, en el nivel más difícil de la CPU.",
        cierre = "La élite saiyajin ha caído entera. ¡Sois los verdaderos guerreros de élite!",
        derrota = "Vegeta: «Los saiyajin de élite no pierden dos veces».",
    },
    {
        id = "z2_cooler",
        saga = "Capítulos secretos",
        secreto = "Saga Freezer",
        titulo = "El hermano mayor",
        pareja = "Goku (Med) y Vegeta",
        dialogo = {
            { "Cooler", "Mi hermano siempre fue débil. Yo tengo una forma más." },
            { "Vegeta", "Otro de esa familia de lagartos..." },
            { "Goku", "Cuidado: dicen que tiene copias de metal." },
        },
        rivales = {
            { personaje = 126, nivel = 4, vida = 260, transforma = 40, nombre = "Cooler" },
            { personaje = 128, nivel = 3, vida = 200, nombre = "Metal Cooler" },
        },
        refuerzos = {
            { personaje = 127, nivel = 4, vida = 320, nombre = "Cooler (Forma final)", detras = 1 },
            { personaje = 128, nivel = 3, vida = 200, nombre = "Metal Cooler", detras = 2 },
        },
        escenario = -1, musica = -1, tiempo = 4,
        intro = "Capítulo secreto. Cooler, el hermano de Freezer, se transforma a mitad de combate y trae sus copias de metal.",
        cierre = "Cooler y sus copias han caído. La familia de Freezer ya no asusta a nadie.",
        derrota = "Cooler: «Esto es lo que pasa cuando se enfrentan a mí».",
    },
    {
        id = "z3_cell",
        saga = "Capítulos secretos",
        secreto = "Saga Cell",
        titulo = "Cell, el perfecto",
        pareja = "Gohan (Joven) y Vegeta",
        dialogo = {
            { "Cell", "He vuelto más perfecto que nunca. Y mis hijos también." },
            { "Gohan", "Esta vez no voy a dudar." },
        },
        rivales = {
            { personaje = 107, nivel = 4, vida = 380, nombre = "Cell (Super Perfecto)" },
            { personaje = 109, nivel = 3, vida = 140, nombre = "Cell Jr." },
        },
        refuerzos = {
            { personaje = 109, nivel = 3, vida = 140, nombre = "Cell Jr.", detras = 2 },
            { personaje = 109, nivel = 3, vida = 140, nombre = "Cell Jr.", detras = 2 },
            { personaje = 109, nivel = 3, vida = 140, nombre = "Cell Jr.", detras = 2 },
        },
        escenario = -1, musica = -1, tiempo = 4,
        intro = "Capítulo secreto. Cell con casi el doble de vida y una cola de Cell Jr. que no se acaba.",
        cierre = "Cell explota de verdad esta vez. Lo habéis hecho perfecto.",
        derrota = "Cell: «Perfecto... como siempre».",
    },
    {
        id = "z4_maldad",
        saga = "Capítulos secretos",
        secreto = "Saga de Bu",
        titulo = "La pura maldad",
        pareja = "Goku (Fin) y Vegeta",
        dialogo = {
            { "Majin Bu (Pura maldad)", "(Se ríe sin parar y estira los brazos.)" },
            { "Vegeta", "Este Bu no piensa. Solo destruye." },
            { "Goku", "Si cae, vuelve en su forma pura. ¡Preparaos!" },
        },
        rivales = {
            { personaje = 192, nivel = 4, vida = 300, transforma = 40, nombre = "Majin Bu (Pura maldad)" },
        },
        refuerzos = {
            { personaje = 117, nivel = 4, vida = 380, nombre = "Bu (Forma pura)", detras = 1 },
        },
        escenario = -1, musica = -1, tiempo = 4,
        intro = "Capítulo secreto. Bu en su forma más malvada; al 40% de vida se convierte en Bu puro.",
        cierre = "No queda nada de Bu. La Tierra por fin descansa.",
        derrota = "Bu se ríe mientras todo se convierte en caramelo.",
    },
    {
        id = "z5_legendario",
        saga = "Capítulos secretos",
        secreto = "Saga de las Películas",
        titulo = "El legendario despierta",
        pareja = "Goku (Fin) y Vegeta",
        dialogo = {
            { "Broly", "Kakarotto... ¡Kakarottooo!" },
            { "Vegeta", "Su poder no deja de crecer. Al final no habrá quien lo pare." },
            { "Goku", "Pues hay que pararlo antes de que se transforme del todo." },
        },
        rivales = {
            { personaje = 189, nivel = 4, vida = 300, transforma = 40, nombre = "Broly (SS)" },
        },
        refuerzos = {
            { personaje = 190, nivel = 4, vida = 400, nombre = "Broly (Legendario)", detras = 1 },
        },
        escenario = -1, musica = -1, tiempo = 4,
        intro = "Capítulo secreto. Broly empieza en Súper Saiyajin y al 40% de vida despierta como el Legendario.",
        cierre = "El Súper Saiyajin Legendario ha caído. Nadie lo creerá.",
        derrota = "Broly ruge. No queda nadie en pie.",
    },
    {
        id = "z6_jiren",
        saga = "Capítulos secretos",
        secreto = "Saga Super",
        titulo = "Más allá de los dioses",
        pareja = "Goku (Fin) y Vegeta",
        dialogo = {
            { "Jiren", "Solo la fuerza tiene valor. Demostradla." },
            { "Toppo", "Como dios de la destrucción, no os dejaré pasar." },
            { "Goku", "¡Este es el combate más difícil de todos!" },
        },
        rivales = {
            { personaje = 151, nivel = 4, vida = 400, nombre = "Jiren" },
            { personaje = 150, nivel = 4, vida = 300, nombre = "Toppo (Hakaishin)" },
        },
        escenario = -1, musica = -1, tiempo = 4,
        intro = "Capítulo secreto. Jiren y Toppo como dios de la destrucción, con toda su vida. El reto final.",
        cierre = "Habéis derrotado a lo más fuerte del Universo 11. No hay nada por encima.",
        derrota = "Jiren se da la vuelta sin decir nada.",
    },
    -- ------------------------------------------------------------------ Saga ¿Y si...? (rutas alternativas)
    -- Cada uno se abre al completar su saga («requiere»).
    {
        id = "w1_vegeta",
        saga = "Saga ¿Y si...?",
        requiere = "Saga Saiyajin",
        titulo = "¿Y si Vegeta no hubiera cambiado?",
        pareja = "Goku (Med) y Piccoro (Fin)",
        dialogo = {
            { "Vegeta", "Nunca me uní a vosotros. Namek será mía, y después la Tierra." },
            { "Goku", "Vegeta... pensaba que habías cambiado." },
            { "Zarbon", "El príncipe y yo trabajamos juntos ahora. Qué sorpresa, ¿verdad?" },
        },
        rivales = {
            { personaje = 31, nivel = 4, vida = 260, nombre = "Vegeta" },
            { personaje = 82, nivel = 3, vida = 180, transforma = 40, nombre = "Zarbon" },
        },
        refuerzos = {
            { personaje = 83, nivel = 3, vida = 220, nombre = "Zarbon (Transformado)", detras = 2 },
            { personaje = 84, nivel = 3, vida = 180, nombre = "Dodoria", detras = 1 },
        },
        escenario = -1, musica = -1, tiempo = 4,
        intro = "Ruta alternativa. Vegeta nunca se unió a los guerreros Z y se ha aliado con los esbirros de Freezer.",
        cierre = "Vegeta cae de rodillas. Quizá algún día cambie de verdad.",
        derrota = "Vegeta: «Os lo dije: soy el príncipe de los saiyajin».",
    },
    {
        id = "w2_freezer",
        saga = "Saga ¿Y si...?",
        requiere = "Saga Freezer",
        titulo = "¿Y si Freezer hubiera ganado en Namek?",
        pareja = "Gohan (Joven) y Krilin",
        dialogo = {
            { "Freezer", "Goku ya no está. Ahora el universo entero me obedece." },
            { "Krilin", "Sin Goku... nos toca a nosotros, Gohan." },
            { "Gohan", "¡Por mi padre!" },
        },
        rivales = {
            { personaje = 95, nivel = 4, vida = 320, nombre = "Freezer (100% de poder)" },
            { personaje = 86, nivel = 3, vida = 200, nombre = "Capitán Ginyu" },
        },
        refuerzos = {
            { personaje = 97, nivel = 3, vida = 220, nombre = "King Cold", detras = 2 },
            { personaje = 96, nivel = 4, vida = 260, nombre = "Mecha Freezer", detras = 1 },
        },
        escenario = -1, musica = -1, tiempo = 4,
        intro = "Ruta alternativa. Goku perdió en Namek y Freezer gobierna. Los que quedan intentan recuperar la Tierra.",
        cierre = "El imperio de Freezer se derrumba. La Tierra vuelve a ser libre.",
        derrota = "Freezer: «Sin Goku no sois nada».",
    },
    {
        id = "w3_cell",
        saga = "Saga ¿Y si...?",
        requiere = "Saga Cell",
        titulo = "¿Y si Cell hubiera absorbido a todos?",
        pareja = "Gohan Definitivo y Vegetto",
        dialogo = {
            { "Cell", "Gohan, Vegeta, Piccoro... todos forman parte de mí ahora." },
            { "Vegetto", "Pues entonces te sacaremos uno por uno." },
        },
        rivales = {
            { personaje = 107, nivel = 4, vida = 400, nombre = "Cell (con todos absorbidos)" },
        },
        refuerzos = {
            { personaje = 108, nivel = 4, vida = 300, nombre = "Cell Max", detras = 1 },
        },
        escenario = -1, musica = -1, tiempo = 4,
        intro = "Ruta alternativa. Cell absorbió a todos los guerreros y ahora es imparable. Cuando caiga, despierta su forma gigante.",
        cierre = "Cell se desintegra y libera a todos. Ha sido por los pelos.",
        derrota = "Cell: «Ahora sí soy perfecto».",
    },
    {
        id = "w4_bu",
        saga = "Saga ¿Y si...?",
        requiere = "Saga de Bu",
        titulo = "¿Y si Bu hubiera ganado?",
        pareja = "Goku (Fin) y Vegeta",
        dialogo = {
            { "Bu", "(La Tierra entera es de caramelo. Bu se ríe.)" },
            { "Vegeta", "Somos los últimos, Kakarotto." },
            { "Goku", "Entonces no podemos fallar." },
        },
        rivales = {
            { personaje = 117, nivel = 4, vida = 360, nombre = "Bu (Forma pura)" },
            { personaje = 114, nivel = 4, vida = 240, nombre = "Súper Bu" },
        },
        refuerzos = {
            { personaje = 116, nivel = 4, vida = 260, nombre = "Súper Bu (Gohan absorbido)", detras = 2 },
        },
        escenario = -1, musica = -1, tiempo = 4,
        intro = "Ruta alternativa. Bu ganó y solo quedáis vosotros dos. Es la última esperanza.",
        cierre = "Bu desaparece. La Tierra empieza de nuevo.",
        derrota = "Bu: «¡Bu gana! ¡Bu gana!».",
    },
    -- ------------------------------------------------------------------ Jefe semanal
    -- Uno distinto cada semana (el mismo para todos), elegido por la fecha entre los marcados «semanal».
    {
        id = "k1_broly",
        saga = "Jefe semanal",
        semanal = true,
        titulo = "Broly sin límites",
        pareja = "Los que queráis",
        dialogo = {
            { "Broly", "¡Kakarottooo!" },
            { "Goku", "¡Es el jefe de la semana! Cuanto antes, mejor nota." },
        },
        rivales = {
            { personaje = 190, nivel = 4, vida = 400, nombre = "Broly (Legendario)" },
            { personaje = 189, nivel = 4, vida = 260, nombre = "Broly (SS)" },
        },
        escenario = -1, musica = -1, tiempo = 4,
        intro = "Jefe semanal: el mismo para todos esta semana. Vida enorme, CPU al máximo y premio triple. ¡Contra el reloj!",
        cierre = "¡Jefe semanal derrotado! Vuelve la semana que viene a por otro.",
        derrota = "El jefe semanal sigue en pie. Tenéis toda la semana.",
    },
    {
        id = "k2_janemba",
        saga = "Jefe semanal",
        semanal = true,
        titulo = "El demonio del caos",
        pareja = "Los que queráis",
        dialogo = {
            { "Janemba", "(Las puertas del Más Allá se abren a su alrededor.)" },
            { "Goku", "¡Es el jefe de la semana! Cuanto antes, mejor nota." },
        },
        rivales = {
            { personaje = 138, nivel = 4, vida = 400, nombre = "Súper Janemba" },
            { personaje = 137, nivel = 4, vida = 240, nombre = "Janemba" },
        },
        escenario = -1, musica = -1, tiempo = 4,
        intro = "Jefe semanal: el mismo para todos esta semana. Vida enorme, CPU al máximo y premio triple. ¡Contra el reloj!",
        cierre = "¡Jefe semanal derrotado! Vuelve la semana que viene a por otro.",
        derrota = "El jefe semanal sigue en pie. Tenéis toda la semana.",
    },
    {
        id = "k3_omega",
        saga = "Jefe semanal",
        semanal = true,
        titulo = "El dragón definitivo",
        pareja = "Los que queráis",
        dialogo = {
            { "Omega Shenron", "Las siete bolas son mías. Y vuestro planeta también." },
            { "Goku", "¡Es el jefe de la semana! Cuanto antes, mejor nota." },
        },
        rivales = {
            { personaje = 147, nivel = 4, vida = 400, nombre = "Omega Shenron" },
            { personaje = 145, nivel = 4, vida = 220, nombre = "Neo Shenron" },
        },
        escenario = -1, musica = -1, tiempo = 4,
        intro = "Jefe semanal: el mismo para todos esta semana. Vida enorme, CPU al máximo y premio triple. ¡Contra el reloj!",
        cierre = "¡Jefe semanal derrotado! Vuelve la semana que viene a por otro.",
        derrota = "El jefe semanal sigue en pie. Tenéis toda la semana.",
    },
    {
        id = "k4_zamas",
        saga = "Jefe semanal",
        semanal = true,
        titulo = "La fusión divina",
        pareja = "Los que queráis",
        dialogo = {
            { "Zamas", "Los mortales sois un error. Yo soy la justicia." },
            { "Goku", "¡Es el jefe de la semana! Cuanto antes, mejor nota." },
        },
        rivales = {
            { personaje = 76, nivel = 4, vida = 400, nombre = "Zamas (Fusión)" },
            { personaje = 34, nivel = 4, vida = 240, nombre = "Black Goku (Rosé)" },
        },
        escenario = -1, musica = -1, tiempo = 4,
        intro = "Jefe semanal: el mismo para todos esta semana. Vida enorme, CPU al máximo y premio triple. ¡Contra el reloj!",
        cierre = "¡Jefe semanal derrotado! Vuelve la semana que viene a por otro.",
        derrota = "El jefe semanal sigue en pie. Tenéis toda la semana.",
    },
    {
        id = "k5_kefla",
        saga = "Jefe semanal",
        semanal = true,
        titulo = "La saiyajin fusionada",
        pareja = "Los que queráis",
        dialogo = {
            { "Kefla", "¡Os voy a enseñar lo que es la fuerza de verdad!" },
            { "Goku", "¡Es el jefe de la semana! Cuanto antes, mejor nota." },
        },
        rivales = {
            { personaje = 154, nivel = 4, vida = 400, nombre = "Kefla (SS)" },
            { personaje = 199, nivel = 4, vida = 240, nombre = "Kale (SS)" },
        },
        escenario = -1, musica = -1, tiempo = 4,
        intro = "Jefe semanal: el mismo para todos esta semana. Vida enorme, CPU al máximo y premio triple. ¡Contra el reloj!",
        cierre = "¡Jefe semanal derrotado! Vuelve la semana que viene a por otro.",
        derrota = "El jefe semanal sigue en pie. Tenéis toda la semana.",
    },
    {
        id = "k6_cooler",
        saga = "Jefe semanal",
        semanal = true,
        titulo = "El regreso de Cooler",
        pareja = "Los que queráis",
        dialogo = {
            { "Cooler", "Esta vez no habrá nadie que os salve." },
            { "Goku", "¡Es el jefe de la semana! Cuanto antes, mejor nota." },
        },
        rivales = {
            { personaje = 127, nivel = 4, vida = 400, nombre = "Cooler (Forma final)" },
            { personaje = 128, nivel = 4, vida = 220, nombre = "Metal Cooler" },
        },
        escenario = -1, musica = -1, tiempo = 4,
        intro = "Jefe semanal: el mismo para todos esta semana. Vida enorme, CPU al máximo y premio triple. ¡Contra el reloj!",
        cierre = "¡Jefe semanal derrotado! Vuelve la semana que viene a por otro.",
        derrota = "El jefe semanal sigue en pie. Tenéis toda la semana.",
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
--   34 Black Goku (Rosé)  70 Golden Freezer  76 Zamas (Fusión)  78 Hit  81 Toppo  99 Black Goku  104 Kefla
--   119 Caulifla (SS)  134 Cabba (SS)  136 Súper Bojack  138 Súper Janemba  140 Baby Vegeta  141 Súper Baby
--   142 Súper Baby (2.ª forma)  144 Súper 17  145 Neo Shenron  146 Eis Shenron  147 Omega Shenron  150 Toppo (Hakaishin)
--   151 Jiren  154 Kefla (SS)  157 Zamas  182 Garlick Jr.  183 Súper Garlick Jr.  186 Salza  187 Zangya  189 Broly (SS)
--   197 Syn Shenron  199 Kale (SS)
--   Gigantes (solos, en 2 contra 1 y sin refuerzos): 12 Ozaru  108 Cell Max  124 Slug (Gigante)  143 Súper Baby (Ozaru)  195 Hildegarn
