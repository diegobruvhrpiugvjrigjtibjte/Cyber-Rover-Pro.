*
 * Cyber Rover Pro - Arduino/ESP32 Firmware
 * Include gestione Motori, Luci e Telemetria stimata.
 */

#include <WiFi.h>
#include <WebServer.h>

// --- CONFIGURAZIONE WIFI ---
const char* ssid = "CyberRover_AP";
const char* password = "password123";

WebServer server(80);

// --- PIN HARDWARE ---
const int motorL1 = 26; 
const int motorL2 = 27;
const int motorR1 = 14;
const int motorR2 = 12;
const int lightPin = 2; // LED integrato o striscia LED

bool lightsOn = false;

// --- PAGINA HTML ---
const char INDEX_HTML[] PROGMEM = R"=====(
<!DOCTYPE html>
<html lang="it">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0, maximum-scale=1.0, user-scalable=no">
    <title>Cyber Rover Pro - Stable Infinite Drive</title>
    <script src="https://cdn.tailwindcss.com"></script>
    <style>
        @import url('https://fonts.googleapis.com/css2?family=Orbitron:wght@400;700;900&display=swap');
        :root { --neon-cyan: #00f3ff; --neon-magenta: #ff00ff; --motor-yellow: #ffd700; --danger-red: #ff3131; }
        body { background-color: #020202; color: #fff; font-family: 'Orbitron', sans-serif; touch-action: none; overflow: hidden; display: flex; flex-direction: column; height: 100vh; }
        .hud-glass { background: linear-gradient(180deg, rgba(15, 23, 42, 0.95) 0%, rgba(2, 6, 23, 1) 100%); border-bottom: 1px solid rgba(0, 243, 255, 0.3); z-index: 100; }
        .speedo-container { position: relative; width: 100px; height: 100px; }
        .speedo-svg { transform: rotate(-220deg); width: 100px; height: 100px; }
        .speedo-bg { fill: none; stroke: rgba(255, 255, 255, 0.05); stroke-width: 8; stroke-dasharray: 210 360; }
        .speedo-fill { fill: none; stroke: var(--neon-cyan); stroke-width: 8; stroke-dasharray: 0 360; stroke-linecap: round; filter: drop-shadow(0 0 8px var(--neon-cyan)); }
        .speedo-needle { position: absolute; width: 2px; height: 40px; background: var(--danger-red); bottom: 50%; left: calc(50% - 1px); transform-origin: bottom center; transform: rotate(-130deg); z-index: 10; box-shadow: 0 0 8px var(--danger-red); }
        .viewport-3d { perspective: 1000px; background: radial-gradient(circle at 50% 50%, #0f172a 0%, #000 100%); flex: 1; position: relative; overflow: hidden; }
        #world-plane { position: absolute; width: 400vw; height: 400vh; top: -150vh; left: -150vw; background-image: linear-gradient(rgba(0, 243, 255, 0.1) 2px, transparent 2px), linear-gradient(90deg, rgba(0, 243, 255, 0.1) 2px, transparent 2px); background-size: 100px 100px; transform: rotateX(70deg); will-change: transform, background-position; }
        #car-center { position: absolute; top: 65%; left: 50%; transform: translate(-50%, -50%); transform-style: preserve-3d; z-index: 50; }
        .chassis-main { width: 60px; height: 90px; background: #1e293b; border: 2px solid var(--neon-cyan); box-shadow: 0 0 30px rgba(0, 243, 255, 0.3); border-radius: 8px; position: relative; transform-style: preserve-3d; }
        .motor-unit { position: absolute; width: 14px; height: 35px; background: var(--motor-yellow); border: 1px solid #000; }
        .wheel-3d { position: absolute; width: 10px; height: 24px; background: #111; border: 1px solid #444; border-radius: 3px; }
        .beam { position: absolute; top: -300px; width: 80px; height: 400px; background: linear-gradient(to top, rgba(0, 243, 255, 0.6), transparent); filter: blur(20px); display: none; transform-origin: bottom; pointer-events: none; }
        .bottom-panel { background: #020202; padding: 1.5rem; display: grid; grid-template-columns: 1fr 1fr; gap: 1.5rem; border-top: 1px solid rgba(255,255,255,0.1); }
        .d-pad { display: grid; grid-template-columns: repeat(3, 1fr); gap: 8px; width: 140px; height: 140px; }
        .control-btn { background: #0f172a; border: 1px solid rgba(0, 243, 255, 0.2); border-radius: 10px; display: flex; align-items: center; justify-content: center; font-size: 1.2rem; color: var(--neon-cyan); cursor: pointer; user-select: none; }
        .control-btn:active, .control-btn.active { background: var(--neon-cyan); color: #000; box-shadow: 0 0 15px var(--neon-cyan); }
        .cyber-btn { background: rgba(255, 0, 255, 0.1); border: 1px solid var(--neon-magenta); color: var(--neon-magenta); clip-path: polygon(5% 0, 100% 0, 95% 100%, 0% 100%); padding: 10px; font-weight: 900; font-size: 0.8rem; width: 100%; transition: 0.2s; }
        .cyber-btn.active { background: var(--neon-magenta); color: white; box-shadow: 0 0 15px var(--neon-magenta); }
    </style>
</head>
<body>
    <div class="hud-glass px-6 py-2 grid grid-cols-3 gap-2 items-center">
        <div class="speedo-container">
            <svg class="speedo-svg" viewBox="0 0 100 100">
                <circle class="speedo-bg" cx="50" cy="50" r="40"></circle>
                <circle id="speedo-fill" class="speedo-fill" cx="50" cy="50" r="40"></circle>
            </svg>
            <div id="needle" class="speedo-needle"></div>
            <div class="absolute inset-0 flex flex-col items-center justify-center pt-8">
                <span id="speed-text" class="text-lg font-black italic text-cyan-400">0.00</span>
                <span class="text-[5px] opacity-40 uppercase">M/S</span>
            </div>
        </div>
        <div class="text-center border-x border-white/5">
            <span class="text-[6px] opacity-30 block uppercase">System Voltage</span>
            <span id="volt-text" class="text-xl font-black text-green-400">4.20</span><span class="text-[8px] text-green-400 ml-1">V</span>
            <div class="w-16 h-1 bg-white/5 mx-auto mt-1 rounded-full overflow-hidden">
                <div id="batt-level" class="h-full bg-green-500" style="width: 100%"></div>
            </div>
        </div>
        <div class="pl-2">
            <span class="text-[6px] opacity-30 block uppercase">Current Draw</span>
            <span id="amp-text" class="text-xl font-black text-magenta-500">0.00</span><span class="text-[8px] text-magenta-500 ml-1">A</span>
        </div>
    </div>

    <div class="viewport-3d">
        <div id="world-plane"></div>
        <div id="car-center">
            <div id="chassis" class="chassis-main">
                <div id="beam-l" class="beam" style="left: 5px;"></div>
                <div id="beam-r" class="beam" style="right: 5px;"></div>
                <div class="motor-unit" style="top: 10px; left: -14px;"></div>
                <div id="w-fl" class="wheel-3d" style="top: 15px; left: -18px;"></div>
                <div class="motor-unit" style="top: 10px; right: -14px;"></div>
                <div id="w-fr" class="wheel-3d" style="top: 15px; right: -18px;"></div>
                <div class="motor-unit" style="bottom: 10px; left: -14px;"></div>
                <div id="w-bl" class="wheel-3d" style="bottom: 15px; left: -18px;"></div>
                <div class="motor-unit" style="bottom: 10px; right: -14px;"></div>
                <div id="w-br" class="wheel-3d" style="bottom: 15px; right: -18px;"></div>
            </div>
        </div>
    </div>

    <div class="bottom-panel">
        <div class="flex justify-center">
            <div class="d-pad">
                <div></div><div class="control-btn" id="btn-up">▲</div><div></div>
                <div class="control-btn" id="btn-left">◀</div>
                <div class="control-btn border-red-900 text-red-600" id="btn-stop">■</div>
                <div class="control-btn" id="btn-right">▶</div>
                <div></div><div class="control-btn" id="btn-down">▼</div><div></div>
            </div>
        </div>
        <div class="flex flex-col gap-3">
            <button id="btn-lights" class="cyber-btn">TOGGLE NEON</button>
            <div class="bg-black/50 p-2 border border-white/5 rounded text-[6px] font-mono opacity-40 uppercase">
                Hardware: ESP32 Connected<br>Engine: Dual-Channel PWM<br>Sensor: Volt/Amp Simulated
            </div>
        </div>
    </div>

    <script>
        const MAX_RPM = 180;
        let state = { posX: 0, posY: 0, angle: 0, currRpm: 0, targetRpm: 0, turn: 0, v: 4.18, a: 0.02, lights: false };
        const world = document.getElementById('world-plane');
        const chassis = document.getElementById('chassis');
        const needle = document.getElementById('needle');
        const speedoFill = document.getElementById('speedo-fill');

        function update() {
            // Logica Fisica Fluida
            state.currRpm += (state.targetRpm - state.currRpm) * 0.1;
            const speedPerc = Math.abs(state.currRpm) / MAX_RPM;
            
            state.angle += state.turn * (speedPerc * 3.5 || 1.2);
            const rad = (state.angle - 90) * (Math.PI / 180);
            const moveStep = (state.currRpm / MAX_RPM) * 12;
            
            state.posX -= Math.cos(rad) * moveStep;
            state.posY -= Math.sin(rad) * moveStep;
            
            world.style.backgroundPosition = `${state.posX}px ${state.posY}px`;
            world.style.transform = `rotateX(70deg) rotateZ(${-state.angle}deg)`;
            
            // Beccheggio dinamico (Sospensioni)
            const pitch = (state.targetRpm - state.currRpm) * 0.15;
            const roll = state.turn * 2.5;
            chassis.style.transform = `rotateX(${pitch}deg) rotateY(${roll}deg)`;
            
            // HUD Tachimetro
            const arc = speedPerc * 148;
            speedoFill.style.strokeDasharray = `${arc} 360`;
            needle.style.transform = `rotate(${-130 + (speedPerc * 210)}deg)`;
            document.getElementById('speed-text').innerText = (speedPerc * 2.4).toFixed(2);
            
            // Telemetria Sensori (Simulata su base hardware)
            let load = speedPerc * 1.8;
            if (state.lights) load += 0.15;
            state.a += (load - state.a) * 0.05;
            state.v -= state.a * 0.000005; // Scarica lenta della batteria
            
            document.getElementById('volt-text').innerText = state.v.toFixed(2);
            document.getElementById('amp-text').innerText = state.a.toFixed(2);
            document.getElementById('batt-level').style.width = `${Math.max(0, ((state.v-3.2)/(4.2-3.2)*100))}%`;

            requestAnimationFrame(update);
        }

        async function sendCommand(dir) {
            try { await fetch(`/cmd?dir=${dir}`); } catch(e) {}
        }

        function cmd(type) {
            if(type === 'LIGHTS') {
                state.lights = !state.lights;
                const beams = document.querySelectorAll('.beam');
                const btn = document.getElementById('btn-lights');
                beams.forEach(b => b.style.display = state.lights ? 'block' : 'none');
                btn.classList.toggle('active', state.lights);
                sendCommand('LIGHTS');
                return;
            }
            
            sendCommand(type);
            switch(type) {
                case 'AVANTI': state.targetRpm = MAX_RPM; state.turn = 0; break;
                case 'INDIETRO': state.targetRpm = -MAX_RPM; state.turn = 0; break;
                case 'DESTRA': state.turn = 3; state.targetRpm = MAX_RPM * 0.6; break;
                case 'SINISTRA': state.turn = -3; state.targetRpm = MAX_RPM * 0.6; break;
                case 'STOP': state.targetRpm = 0; state.turn = 0; break;
            }
        }

        const setup = (id, c) => {
            const el = document.getElementById(id);
            const start = (e) => { e.preventDefault(); cmd(c); el.classList.add('active'); };
            const end = (e) => { e.preventDefault(); el.classList.remove('active'); if(c !== 'STOP') cmd('STOP'); };
            el.addEventListener('mousedown', start); el.addEventListener('touchstart', start);
            el.addEventListener('mouseup', end); el.addEventListener('touchend', end);
            el.addEventListener('mouseleave', end);
        };

        setup('btn-up','AVANTI'); setup('btn-down','INDIETRO');
        setup('btn-left','SINISTRA'); setup('btn-right','DESTRA');
        document.getElementById('btn-stop').onclick = () => cmd('STOP');
        document.getElementById('btn-lights').onclick = () => cmd('LIGHTS');

        requestAnimationFrame(update);
    </script>
</body>
</html>
)=====";

// --- LOGICA SERVER ---
void drive(int l1, int l2, int r1, int r2) {
  digitalWrite(motorL1, l1); digitalWrite(motorL2, l2);
  digitalWrite(motorR1, r1); digitalWrite(motorR2, r2);
}

void handleCommand() {
  if (server.hasArg("dir")) {
    String d = server.arg("dir");
    if (d == "AVANTI") drive(1, 0, 1, 0);
    else if (d == "INDIETRO") drive(0, 1, 0, 1);
    else if (d == "SINISTRA") drive(0, 1, 1, 0); 
    else if (d == "DESTRA") drive(1, 0, 0, 1);
    else if (d == "STOP") drive(0, 0, 0, 0);
    else if (d == "LIGHTS") {
      lightsOn = !lightsOn;
      digitalWrite(lightPin, lightsOn ? HIGH : LOW);
    }
  }
  server.send(200, "text/plain", "OK");
}

void setup() {
  Serial.begin(115200);
  pinMode(motorL1, OUTPUT); pinMode(motorL2, OUTPUT);
  pinMode(motorR1, OUTPUT); pinMode(motorR2, OUTPUT);
  pinMode(lightPin, OUTPUT);
  
  drive(0, 0, 0, 0);
  digitalWrite(lightPin, LOW);

  WiFi.softAP(ssid, password);
  Serial.println("Cyber Rover AP Start");
  Serial.print("IP: "); Serial.println(WiFi.softAPIP());

  server.on("/", []() { server.send_P(200, "text/html", INDEX_HTML); });
  server.on("/cmd", handleCommand);
  server.begin();
}

void loop() {
  server.handleClient();
}