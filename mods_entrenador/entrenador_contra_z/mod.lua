-- Entrenador de defensa: te dice si la contra Z (arriba + cuadrado) y la esquiva con círculo fueron a tiempo,
-- antes o tarde, en ticks de la lógica del juego (30 por segundo, unos 33 ms cada uno).
-- Necesita el juego compilado con el parche del entrenador, que añade fighter.defence a la API de mods.
-- Esquiva con círculo: el juego abre a la vez tres ventanas con una sola pulsación (golpes cuerpo a cuerpo,
-- técnicas y embestidas); el mod te dice cuál se usó.

local MS = 1000 / 30 -- ms por tick
local LOOKBACK = 34  -- ticks hacia atrás que cuentan como intento (la espera tras pulsar)
local LATE = 15      -- ticks después del golpe en los que una pulsación cuenta como "tarde"

-- grupo 1 = contra Z, grupo 2 = esquiva con círculo
local KINDS = {
    { key = "zcounter", name = "Contra Z", group = 1 },
    { key = "dodge", name = "Fium fium (golpe)", group = 2 },
    { key = "guard", name = "Fium fium (técnica)", group = 2 },
    { key = "rush", name = "Fium fium (embestida)", group = 2 },
}
local GROUPS = { "Contra Z", "Fium fium" }

local s = { reset = false, flash = -1000, flashText = "", flashSub = "", flashColor = 0, k = {}, g = {}, unlockedSeq = nil }
-- Modo examen: los 20 siguientes intentos (contra Z o fium fium) cuentan; al final, una nota.
local EXAM_TOTAL = 20
local exam = { on = false, active = false, n = 0, ok = 0, result = nil, resultAt = -1000, best = nil }
local function grade(pct)
    return pct >= 95 and "Z" or pct >= 85 and "S" or pct >= 70 and "A" or pct >= 55 and "B" or pct >= 40 and "C" or pct >= 25 and "D" or "E"
end

local function clear()
    for i = 1, #KINDS do s.k[i] = { pressSeq = nil, hitSeq = nil, pending = nil } end
    for i = 1, #GROUPS do s.g[i] = { ok = 0, early = 0, late = 0, locked = 0, miss = 0, streak = 0, best = 0, log = {} } end
    s.flash = -1000
end
clear()

local GREEN, ORANGE, RED, BLUE, GREY = 0x6EFF8CFF, 0xFFB450FF, 0xFF6E5AFF, 0x8CC8FFFF, 0xC8C8C8FF

-- streak: 1 = otro acierto seguido, -1 = se corta la racha, nil = no cuenta (p. ej. NO PERMITIDO)
local function report(game, i, text, sub, color, counter, streak)
    local kind = KINDS[i]
    local g = s.g[kind.group]
    if counter then g[counter] = g[counter] + 1 end
    if exam.active and counter then -- (el examen: cada intento juzgado)
        exam.n = exam.n + 1
        if counter == "ok" and streak == 1 then exam.ok = exam.ok + 1 end
        if exam.n >= EXAM_TOTAL then
            exam.active = false
            local pct = math.floor(exam.ok * 100 / EXAM_TOTAL + 0.5)
            exam.result = { ok = exam.ok, pct = pct, grade = grade(pct) }
            exam.resultAt = game.frame
            if not exam.best or pct > exam.best then exam.best = pct end
        end
    end
    local milestone = false
    if streak == 1 then
        g.streak = g.streak + 1
        if g.streak > g.best then g.best = g.streak end
        sub = sub .. string.format("  ·  racha %d", g.streak)
        milestone = g.streak % 5 == 0
    elseif streak == -1 then
        if g.streak >= 3 then sub = sub .. string.format("  ·  racha cortada en %d", g.streak) end
        g.streak = 0
    end
    local label = kind.group == 2 and (kind.name:match("%((.-)%)") .. ": ") or ""
    table.insert(g.log, 1, { text = label .. text .. (sub ~= "" and (" · " .. sub) or ""), color = color })
    if #g.log > 6 then g.log[7] = nil end
    s.flash, s.flashText, s.flashSub, s.flashColor = game.frame, kind.name .. ": " .. text, sub, color
    if milestone then
        s.flashText = string.format("¡RACHA DE %d %s!", g.streak, kind.group == 2 and "FIUM FIUMS" or "CONTRAS Z")
        s.flashSub = string.format("mejor racha: %d", g.best)
        s.flashColor = 0xFFD55BFF
    end
end

local function ticks(n)
    return string.format("%d tick%s (%d ms)", n, n == 1 and "" or "s", math.floor(n * MS + 0.5))
end

local function mySide(game)
    local pick = bt3.setting("side")
    if pick == 2 then return 1 end
    if pick == 3 then return 2 end
    if game.me then return game.me end
    local f1, f2 = game.fighters[1], game.fighters[2]
    if f1 and f1.cpu and f2 and not f2.cpu then return 2 end
    return 1
end

local function update(game, i, d)
    local k = s.k[i]
    local newPress = k.pressSeq ~= nil and d.press_seq ~= k.pressSeq
    local newHit = k.hitSeq ~= nil and d.hit_seq ~= k.hitSeq
    k.pressSeq, k.hitSeq = d.press_seq, d.hit_seq

    -- Una pulsación después de un golpe fallado: tarde.
    if newPress and k.pending and not newHit then
        local late = d.press_tick - k.pending
        if late >= 1 and late <= LATE then
            report(game, i, "TARDE", ticks(late) .. " después del golpe", ORANGE, "late", -1)
        end
        k.pending = nil
    end
    if k.pending and d.tick - k.pending > LATE then
        if bt3.setting("miss") then report(game, i, "SIN PULSAR", "no reaccionaste a este ataque", GREY, "miss", -1) end
        k.pending = nil
    end

    if not newHit then return end
    local since = d.press_seq > 0 and d.hit_tick - d.press_tick or 1e9

    if d.hit_ok and d.hit_eligible then
        local w = math.max(d.press_window, 1)
        if d.hit_noki then
            report(game, i, "A TIEMPO, PERO SIN KI", "esta esquiva cuesta 5000 de Ki", ORANGE, "ok", -1)
        else
            report(game, i, "¡A TIEMPO!", string.format("tick %d de %d de la ventana", since + 1, w), GREEN, "ok", 1)
        end
        return
    end
    if since > LOOKBACK then
        k.pending = d.hit_tick -- quizá llegue tarde
        return
    end
    if not d.hit_eligible then
        report(game, i, "NO PERMITIDO", KINDS[i].group == 1 and "tu animación no admitía contra Z"
                                        or "tu animación no admitía esquivar", BLUE)
        return
    end
    if d.press_locked then
        report(game, i, "BLOQUEADO", "volviste a pulsar durante la espera", RED, "locked", -1)
        return
    end
    local w = math.max(d.press_window, 1)
    report(game, i, "ANTES DE TIEMPO", ticks(since - (w - 1)) .. " demasiado pronto", ORANGE, "early", -1)
end

local function rgba(c, a)
    return bt3.rgba((c >> 24) & 255, (c >> 16) & 255, (c >> 8) & 255, math.floor(255 * a))
end

return {
    name = "Entrenador de defensa",
    version = "3.2",
    author = "",
    description = "Contra Z y esquiva con círculo (golpes, técnicas y embestidas): a tiempo, antes o tarde, en ticks. Necesita el juego parcheado.",
    settings = {
        { id = "side", label = "Tu lado", type = "choice", choices = { "Auto", "Jugador 1", "Jugador 2" }, default = 1,
          tip = "Auto: el lado que no maneja la CPU." },
        { id = "flash", label = "Aviso grande en pantalla", type = "toggle", default = true },
        { id = "flash_time", label = "Duración del aviso (s)", type = "slider", min = 1, max = 8, step = 1, default = 3 },
        { id = "miss", label = "Avisar si no pulsas", type = "toggle", default = true,
          tip = "Muestra SIN PULSAR cuando te golpean sin que hayas pulsado." },
        { id = "panel", label = "Panel con historial", type = "toggle", default = true },
        { id = "panel_pos", label = "Posición del panel", type = "choice",
          choices = { "Abajo centro", "Abajo izquierda", "Abajo derecha" }, default = 2 },
        { id = "lines", label = "Líneas de historial", type = "slider", min = 0, max = 5, step = 1, default = 2 },
        { id = "opacity", label = "Opacidad del panel", type = "slider", min = 40, max = 100, step = 5, default = 90 },
        { id = "examen", label = "Modo examen (20 intentos y una nota)", type = "toggle", default = false,
          tip = "Actívalo para empezar: los 20 siguientes intentos de contra Z o fium fium cuentan. Desactívalo y actívalo para repetir." },
        { id = "mezcla", label = "CPU: mezcla golpes y especiales al azar", type = "toggle", default = false,
          tip = "Con el entrenamiento de esquiva (F1 > Técnicas) activo, la CPU mezcla golpes y ataques especiales sin avisar." },
        { id = "reset", label = "Reiniciar contadores (cambiar)", type = "toggle", default = false,
          tip = "Cámbialo de posición para poner los contadores a cero." },
    },

    on_frame = function(game)
        local r = bt3.setting("reset")
        if r ~= s.reset then s.reset = r; clear() end
        local e = bt3.setting("examen")
        if e ~= exam.on then
            exam.on = e
            exam.active, exam.n, exam.ok, exam.result = e, 0, 0, nil
        end
        if not game.battle then
            for i = 1, #KINDS do s.k[i].pressSeq, s.k[i].hitSeq, s.k[i].pending = nil, nil, nil end
            s.unlockedSeq = nil
            return
        end
        local f = game.fighters[mySide(game)]
        if not f or not f.defence then return end
        for i, kind in ipairs(KINDS) do
            local d = f.defence[kind.key]
            if d then update(game, i, d) end
        end
        -- círculo sin fijar al rival: el juego no abre ninguna ventana
        local d = f.defence.dodge
        if d and d.unlocked_seq then
            if s.unlockedSeq ~= nil and d.unlocked_seq ~= s.unlockedSeq then
                s.flash, s.flashText, s.flashSub, s.flashColor = game.frame, "Fium fium: SIN FIJAR",
                    "sin fijar al rival (lock-on) el círculo no esquiva", BLUE
            end
            s.unlockedSeq = d.unlocked_seq
        end
    end,

    on_hud = function(hud, game)
        if not game.battle or not game.hud_shown then return end
        local f = game.fighters[mySide(game)]

        if f and not f.defence then
            hud.text(hud.w / 2, hud.h - 40, "Entrenador de defensa: este ejecutable no tiene el parche",
                     rgba(RED, 1), 13, "center", { outline = rgba(0x000000FF, 1) })
            return
        end

        if bt3.setting("panel") then
            local a = bt3.setting("opacity") / 100
            local lines = bt3.setting("lines")
            local w, rowH = 300, 13
            local block = 15 + lines * rowH
            local h = 4 + #GROUPS * block
            local pos = bt3.setting("panel_pos")
            local x = pos == 2 and 8 or pos == 3 and (hud.w - w - 8) or (hud.w / 2 - w / 2)
            local y = hud.h - h - 8
            hud.fill(x, y, w, h, bt3.rgba(0, 0, 0, math.floor(235 * a)), 5)
            for gi, gname in ipairs(GROUPS) do
                local g = s.g[gi]
                local cy = y + 3 + (gi - 1) * block
                hud.text(x + 6, cy, gname, rgba(0xFFD55BFF, a), 11)
                if g.streak > 0 or g.best > 0 then
                    hud.text(x + 66, cy + 1, string.format("racha %d (mejor %d)", g.streak, g.best),
                             rgba(g.streak >= 5 and 0x6EFF8CFF or 0xFFFFFFFF, a), 10)
                end
                hud.text(x + w - 6, cy, string.format("OK %d  Antes %d  Tarde %d  Bloq %d", g.ok, g.early, g.late, g.locked),
                         rgba(0xFFFFFFFF, a), 10, "right")
                for j = 1, lines do
                    local e = g.log[j]
                    if e then
                        hud.text(x + 10, cy + 2 + j * rowH, e.text, rgba(e.color, a * (1 - (j - 1) * 0.15)), 10, "left",
                                 { fit = w - 16 })
                    end
                end
            end
        end

        if exam.active then
            hud.fill(hud.w / 2 - 110, 8, 220, 40, bt3.rgba(0, 0, 0, 210), 6)
            hud.text(hud.w / 2, 12, string.format("EXAMEN  %d / %d", exam.n, EXAM_TOTAL), rgba(0xFFD55BFF, 1), 15, "center")
            hud.text(hud.w / 2, 31, string.format("a tiempo: %d", exam.ok), rgba(0xFFFFFFFF, 1), 11, "center")
        elseif exam.result and game.frame - exam.resultAt < 60 * 12 then
            local o = { outline = bt3.rgba(0, 0, 0, 230), outline_size = 2, font = "game" }
            hud.fill(hud.w / 2 - 180, hud.h * 0.18 - 10, 360, 110, bt3.rgba(0, 0, 0, 200), 10)
            hud.text(hud.w / 2, hud.h * 0.18, "EXAMEN TERMINADO", rgba(0xFFD55BFF, 1), 22, "center", o)
            hud.text(hud.w / 2, hud.h * 0.18 + 30, string.format("Nota %s", exam.result.grade), rgba(0x6EFF8CFF, 1), 34, "center", o)
            hud.text(hud.w / 2, hud.h * 0.18 + 72, string.format("%d de %d a tiempo (%d%%)   mejor: %d%%", exam.result.ok, EXAM_TOTAL,
                     exam.result.pct, exam.best or 0), rgba(0xFFFFFFFF, 1), 13, "center", o)
        end

        local dur = bt3.setting("flash_time") * 60 -- on_hud va a 60 por segundo
        local age = game.frame - s.flash
        if bt3.setting("flash") and age < dur then
            local fade = age < dur - 30 and 1 or (dur - age) / 30 -- se desvanece en el último medio segundo
            local o = { outline = bt3.rgba(0, 0, 0, math.floor(220 * fade)), outline_size = 2, font = "game" }
            hud.text(hud.w / 2, hud.h * 0.28, s.flashText, rgba(s.flashColor, fade), 26, "center", o)
            if s.flashSub ~= "" then
                hud.text(hud.w / 2, hud.h * 0.28 + 30, s.flashSub, rgba(0xFFFFFFFF, fade), 15, "center", o)
            end
        end
    end,
}
