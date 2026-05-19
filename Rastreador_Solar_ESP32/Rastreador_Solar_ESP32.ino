#include <Arduino.h>
#include <ESP32Servo.h>
#include <WebServer.h>
#include <WiFi.h>

// --- Configurações da Rede Wi-Fi ---
// 1. Configuração do seu Roteador (Para acessar sem desconectar a internet)
const char *ssid = "NOME_DO_SEU_ROTEADOR";
const char *password = "SENHA_DO_SEU_ROTEADOR";

// 2. Ponto de Acesso de Emergência (Caso o ESP32 não ache o seu roteador)
const char *ap_ssid = "Robotica";
const char *ap_password = "robo0025"; // Mínimo 8 caracteres

// Porta do Servidor Web
WebServer server(80);

// ======================================================
//                 PINOS DOS SERVOS
// ======================================================
const int servoPinHoriz = 5; // Servo Horizontal
const int servoPinVert = 18; // Servo Vertical

// ======================================================
//                  PINOS DOS LDRs
// ======================================================
const int ldrTL = 32; // Top Left
const int ldrTR = 33; // Top Right
const int ldrBL = 34; // Bottom Left
const int ldrBR = 35; // Bottom Right

// ======================================================
//                OBJETOS DOS SERVOS
// ======================================================
Servo servoHorizontal;
Servo servoVertical;

// ======================================================
//              POSIÇÕES INICIAIS / VARIÁVEIS
// ======================================================
int posHoriz = 90;
int posVert = 90;

// Variáveis de leitura global para telemetria Web
int valTL = 0;
int valTR = 0;
int valBL = 0;
int valBR = 0;

int mediaCima = 0;
int mediaBaixo = 0;
int mediaEsquerda = 0;
int mediaDireita = 0;

// ======================================================
//             LIMITES DE SEGURANÇA
// ======================================================
const int limitHorizMin = 10;
const int limitHorizMax = 170;

const int limitVertMin = 30;
const int limitVertMax = 150;

// ======================================================
//                 CONFIGURAÇÕES
// ======================================================
// Diferença mínima para mover (não-const para permitir ajuste via Web)
int tolerancia = 50;

// Velocidade máxima do movimento
const int velocidadeMax = 5;

// Modo de operação (Automático/Manual) alternável via Web
bool modoAutomatico = true;

// Controle de tempo
unsigned long ultimoMovimento = 0;
const int intervalo = 50;

// Protótipos das rotas web
void handleRoot();
void handleData();
void handleControl();

// ======================================================
//            FUNÇÃO DE MÉDIA DOS LDRs
// ======================================================
int lerMedia(int pino) {
  int soma = 0;
  for (int i = 0; i < 10; i++) {
    soma += analogRead(pino);
  }
  return soma / 10;
}

// ======================================================
//                        SETUP
// ======================================================
void setup() {
  Serial.begin(115200);

  Serial.println("\n=================================");
  Serial.println(" RASTREADOR SOLAR INICIADO ");
  Serial.println("=================================");

  // Resolução ADC do ESP32 (0 - 4095)
  analogReadResolution(12);

  // Inicializa servos
  servoHorizontal.attach(servoPinHoriz);
  servoVertical.attach(servoPinVert);

  // Posição inicial
  servoHorizontal.write(posHoriz);
  servoVertical.write(posVert);

  // Configuração Híbrida de Rede Wi-Fi
  WiFi.mode(WIFI_AP_STA); // Habilita modo Cliente (STA) e Ponto de Acesso (AP)
  
  Serial.print("Tentando conectar no Wi-Fi do roteador: ");
  Serial.println(ssid);
  WiFi.begin(ssid, password);

  // Aguarda até 10 segundos para conectar
  int tentativas = 0;
  while (WiFi.status() != WL_CONNECTED && tentativas < 20) {
    delay(500);
    Serial.print(".");
    tentativas++;
  }
  Serial.println();

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("Conectado ao roteador com sucesso!");
    Serial.print("Conecte-se em http://");
    Serial.println(WiFi.localIP());
  } else {
    Serial.println("Falha ao conectar no roteador. Iniciando AP de emergência...");
    WiFi.softAP(ap_ssid, ap_password);
    Serial.print("Conecte-se na rede '");
    Serial.print(ap_ssid);
    Serial.print("' e acesse http://");
    Serial.println(WiFi.softAPIP());
  }

  // Vinculação de Rotas Web
  server.on("/", HTTP_GET, handleRoot);
  server.on("/data", HTTP_GET, handleData);
  server.on("/control", HTTP_POST, handleControl);

  server.begin();
  Serial.println("Servidor Web ativo!");
  delay(1000);
}

// ======================================================
//                         LOOP
// ======================================================
void loop() {
  // Escuta requisições do painel web constantemente (sem bloqueios)
  server.handleClient();

  // Atualiza apenas no intervalo definido
  if (millis() - ultimoMovimento >= intervalo) {
    ultimoMovimento = millis();

    // ==================================================
    // LEITURA DOS LDRs (Média estável)
    // ==================================================
    valTL = lerMedia(ldrTL);
    valTR = lerMedia(ldrTR);
    valBL = lerMedia(ldrBL);
    valBR = lerMedia(ldrBR);

    // ==================================================
    // MÉDIAS DOS LADOS
    // ==================================================
    mediaCima = (valTL + valTR) / 2;
    mediaBaixo = (valBL + valBR) / 2;
    mediaEsquerda = (valTL + valBL) / 2;
    mediaDireita = (valTR + valBR) / 2;

    // ==================================================
    // DIFERENÇAS E LUZ TOTAL
    // ==================================================
    int difVert = mediaCima - mediaBaixo;
    int difHoriz = mediaEsquerda - mediaDireita;
    int luzTotal = valTL + valTR + valBL + valBR;

    // Se estiver no modo automático, o ESP32 rastreia ativamente
    if (modoAutomatico) {
      // ==================================================
      // MODO NOTURNO
      // ==================================================
      if (luzTotal < 1000) {
        Serial.println("Pouca luz - modo espera");
        // Sai da lógica de movimento neste ciclo
      } else {
        // ==================================================
        // VELOCIDADE PROPORCIONAL
        // ==================================================
        int velocidadeHoriz = map(abs(difHoriz), 0, 4095, 1, velocidadeMax);
        int velocidadeVert = map(abs(difVert), 0, 4095, 1, velocidadeMax);

        // ==================================================
        // MOVIMENTO VERTICAL
        // ==================================================
        if (abs(difVert) > tolerancia) {
          if (mediaCima > mediaBaixo) {
            posVert -= velocidadeVert;
          } else {
            posVert += velocidadeVert;
          }
        }

        // ==================================================
        // MOVIMENTO HORIZONTAL
        // ==================================================
        if (abs(difHoriz) > tolerancia) {
          if (mediaEsquerda > mediaDireita) {
            posHoriz += velocidadeHoriz;
          } else {
            posHoriz -= velocidadeHoriz;
          }
        }

        // ==================================================
        // LIMITES DOS SERVOS
        // ==================================================
        posHoriz = constrain(posHoriz, limitHorizMin, limitHorizMax);
        posVert = constrain(posVert, limitVertMin, limitVertMax);

        // ==================================================
        // ENVIA MOVIMENTO (Apenas se houver alteração)
        // ==================================================
        static int ultimaHoriz = -1;
        static int ultimaVert = -1;

        if (ultimaHoriz != posHoriz) {
          servoHorizontal.write(posHoriz);
          ultimaHoriz = posHoriz;
        }

        if (ultimaVert != posVert) {
          servoVertical.write(posVert);
          ultimaVert = posVert;
        }
      }
    }

    // ==================================================
    // MONITOR SERIAL
    // ==================================================
    Serial.print("TL: ");
    Serial.print(valTL);
    Serial.print(" | TR: ");
    Serial.print(valTR);
    Serial.print(" | BL: ");
    Serial.print(valBL);
    Serial.print(" | BR: ");
    Serial.print(valBR);
    Serial.print(" || H: ");
    Serial.print(posHoriz);
    Serial.print(" | V: ");
    Serial.print(posVert);
    Serial.print(" || DifH: ");
    Serial.print(difHoriz);
    Serial.print(" | DifV: ");
    Serial.println(difVert);
  }
}

// =========================================================
//           INTERFACE GRÁFICA DO PAINEL WEB (HTML/CSS/JS)
// =========================================================
void handleRoot() {
  String html = R"rawliteral(
<!DOCTYPE html>
<html lang="pt-BR">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>Telemetria - Rastreador Solar IoT</title>
    <style>
        :root {
            --primary: #006837;
            --primary-light: #39B54A;
            --bg: #121212;
            --surface: #1e1e1e;
            --text: #f5f5f5;
            --accent: #ffc107;
        }
        * { box-sizing: border-box; margin: 0; padding: 0; font-family: 'Segoe UI', Tahoma, Geneva, Verdana, sans-serif; }
        body { background-color: var(--bg); color: var(--text); padding: 20px; display: flex; flex-direction: column; align-items: center; min-height: 100vh; }
        
        header { text-align: center; margin-bottom: 25px; width: 100%; max-width: 900px; border-bottom: 2px solid var(--primary); padding-bottom: 15px; }
        header h1 { color: var(--primary-light); font-size: 1.8rem; text-transform: uppercase; letter-spacing: 1px; }
        header p { color: #aaa; font-size: 0.9rem; margin-top: 5px; }
        
        .container { display: grid; grid-template-columns: 1fr; gap: 20px; width: 100%; max-width: 900px; }
        @media (min-width: 768px) {
            .container { grid-template-columns: 1fr 1fr; }
        }
        
        .card { background-color: var(--surface); border-radius: 12px; padding: 20px; box-shadow: 0 4px 15px rgba(0,0,0,0.5); border: 1px solid #333; position: relative; }
        .card h2 { font-size: 1.1rem; color: var(--primary-light); margin-bottom: 15px; text-transform: uppercase; border-left: 4px solid var(--primary); padding-left: 8px; }
        
        /* Mapa Solar (Quadrantes) */
        .solar-map { display: grid; grid-template-columns: 1fr 1fr; gap: 10px; width: 220px; height: 220px; margin: 0 auto 15px; }
        .quadrant { display: flex; flex-direction: column; justify-content: center; align-items: center; border-radius: 8px; background: #2a2a2a; transition: background-color 0.3s; font-size: 0.8rem; font-weight: bold; text-shadow: 1px 1px 2px black; padding: 5px; text-align: center; }
        
        /* Representação Virtual do Painel */
        .panel-3d-container { height: 180px; display: flex; justify-content: center; align-items: center; perspective: 400px; background: #151515; border-radius: 8px; overflow: hidden; position: relative; }
        .virtual-panel { width: 110px; height: 140px; background: linear-gradient(135deg, #1d3557 0%, #457b9d 100%); border: 3px solid #666; border-radius: 6px; box-shadow: 0 10px 20px rgba(0,0,0,0.5); display: flex; flex-direction: column; align-items: center; justify-content: space-around; transform-style: preserve-3d; transition: transform 0.2s ease-out; }
        .panel-grid { width: 90%; height: 90%; display: grid; grid-template-columns: repeat(3, 1fr); grid-template-rows: repeat(4, 1fr); gap: 2px; background: rgba(0,0,0,0.4); padding: 4px; border-radius: 4px; }
        .panel-cell { background: rgba(255,255,255,0.1); border-radius: 2px; border: 1px solid rgba(255,255,255,0.05); }
        
        /* Controles */
        .control-group { display: flex; flex-direction: column; gap: 12px; }
        .switch-container { display: flex; align-items: center; justify-content: space-between; background: #252525; padding: 12px; border-radius: 8px; }
        .switch { position: relative; display: inline-block; width: 50px; height: 26px; }
        .switch input { opacity: 0; width: 0; height: 0; }
        .slider-round { position: absolute; cursor: pointer; top: 0; left: 0; right: 0; bottom: 0; background-color: #555; transition: .4s; border-radius: 34px; }
        .slider-round:before { position: absolute; content: ""; height: 18px; width: 18px; left: 4px; bottom: 4px; background-color: white; transition: .4s; border-radius: 50%; }
        input:checked + .slider-round { background-color: var(--primary-light); }
        input:checked + .slider-round:before { transform: translateX(24px); }
        
        .manual-slider { display: flex; flex-direction: column; gap: 5px; background: #252525; padding: 12px; border-radius: 8px; }
        .manual-slider label { font-size: 0.85rem; color: #ccc; display: flex; justify-content: space-between; }
        .manual-slider input[type="range"] { -webkit-appearance: none; width: 100%; height: 6px; background: #444; border-radius: 3px; outline: none; margin: 8px 0; }
        .manual-slider input[type="range"]::-webkit-slider-thumb { -webkit-appearance: none; appearance: none; width: 18px; height: 18px; background: var(--primary-light); border-radius: 50%; cursor: pointer; }
        
        .disabled-overlay { position: absolute; top: 0; left: 0; right: 0; bottom: 0; background: rgba(0,0,0,0.7); display: flex; align-items: center; justify-content: center; z-index: 10; border-radius: 12px; font-weight: bold; color: #ffc107; font-size: 1rem; pointer-events: none; opacity: 0; transition: opacity 0.3s; }
        .disabled .disabled-overlay { pointer-events: auto; opacity: 1; }
        
        /* Métricas */
        .metrics-grid { display: grid; grid-template-columns: 1fr 1fr; gap: 10px; }
        .metric-box { background: #252525; padding: 12px; border-radius: 8px; text-align: center; }
        .metric-val { font-size: 1.4rem; font-weight: bold; color: var(--accent); margin-top: 5px; }
        .metric-label { font-size: 0.75rem; color: #aaa; text-transform: uppercase; }

        footer { text-align: center; margin-top: 30px; color: #666; font-size: 0.8rem; width: 100%; max-width: 900px; border-top: 1px solid #333; padding-top: 15px; }
    </style>
</head>
<body>

    <header>
        <h1>Placa Solar Seguidora do Sol</h1>
        <p>Incubadora de Inovação Social em Robótica e Sustentabilidade - Maricá - RJ</p>
    </header>

    <div class="container">
        
        <!-- CARD DO MAPA DE CALOR SOLAR -->
        <div class="card">
            <h2>Incidência Solar (Sensores)</h2>
            <div class="solar-map">
                <div id="qTL" class="quadrant">TL<br><span id="vTL">0</span></div>
                <div id="qTR" class="quadrant">TR<br><span id="vTR">0</span></div>
                <div id="qBL" class="quadrant">BL<br><span id="vBL">0</span></div>
                <div id="qBR" class="quadrant">BR<br><span id="vBR">0</span></div>
            </div>
            <div style="text-align: center; font-size: 0.85rem; color: #aaa;">
                Os quadrantes brilham proporcionalmente à intensidade da luz.
            </div>
        </div>

        <!-- CARD DA ORIENTAÇÃO VIRTUAL DO PAINEL -->
        <div class="card">
            <h2>Orientação Física do Painel</h2>
            <div class="panel-3d-container">
                <div id="vPanel" class="virtual-panel">
                    <div class="panel-grid">
                        <div class="panel-cell"></div><div class="panel-cell"></div><div class="panel-cell"></div>
                        <div class="panel-cell"></div><div class="panel-cell"></div><div class="panel-cell"></div>
                        <div class="panel-cell"></div><div class="panel-cell"></div><div class="panel-cell"></div>
                        <div class="panel-cell"></div><div class="panel-cell"></div><div class="panel-cell"></div>
                    </div>
                </div>
            </div>
            <div class="metrics-grid" style="margin-top: 15px;">
                <div class="metric-box">
                    <div class="metric-label">Ângulo Base (H)</div>
                    <div id="txtH" class="metric-val">90°</div>
                </div>
                <div class="metric-box">
                    <div class="metric-label">Ângulo Topo (V)</div>
                    <div id="txtV" class="metric-val">90°</div>
                </div>
            </div>
        </div>

        <!-- CARD DE CONTROLE E CONFIGURAÇÃO -->
        <div class="card" id="ctrlCard">
            <h2>Modo de Operação e Ajustes</h2>
            <div class="control-group">
                <div class="switch-container">
                    <span>Rastreamento Automático</span>
                    <label class="switch">
                        <input type="checkbox" id="chkAuto" checked onchange="toggleAuto(this.checked)">
                        <span class="slider-round"></span>
                    </label>
                </div>

                <div class="manual-slider">
                    <label>Ajuste de Sensibilidade (Tolerância) <span id="vTol">50</span></label>
                    <input type="range" id="rngTol" min="10" max="300" value="50" oninput="updateTolerance(this.value)">
                </div>
            </div>
        </div>

        <!-- CARD DE CONTROLE MANUAL (DESATIVADO NO AUTOMÁTICO) -->
        <div class="card" id="manualCard">
            <div class="disabled-overlay" id="manualOverlay">Disponível no Modo Manual</div>
            <h2>Controle Manual dos Servos</h2>
            <div class="control-group">
                <div class="manual-slider">
                    <label>Direção Horizontal (Eixo Base) <span id="lblManH">90°</span></label>
                    <input type="range" id="rngManH" min="10" max="170" value="90" oninput="moveServo('H', this.value)">
                </div>
                <div class="manual-slider">
                    <label>Inclinacão Vertical (Eixo Topo) <span id="lblManV">90°</span></label>
                    <input type="range" id="rngManV" min="30" max="150" value="90" oninput="moveServo('V', this.value)">
                </div>
            </div>
        </div>

    </div>

    <footer>
        <p>Projeto Desenvolvido pela Equipe de Alunos Orientados pelo Prof. Rafael Costa &bull; 2026</p>
    </footer>

    <script>
        async function updateTelemetry() {
            try {
                const response = await fetch('/data');
                const data = await response.json();
                
                // 1. Atualiza leituras analógicas nos quadrantes (LDRs)
                document.getElementById('vTL').innerText = data.tl;
                document.getElementById('vTR').innerText = data.tr;
                document.getElementById('vBL').innerText = data.bl;
                document.getElementById('vBR').innerText = data.br;

                // 2. Calcula as cores de calor (fundo) baseado na luz
                const getHeatColor = (val) => {
                    const ratio = val / 4095;
                    return `rgba(255, 193, 7, ${ratio})`;
                };
                
                document.getElementById('qTL').style.backgroundColor = getHeatColor(data.tl);
                document.getElementById('qTR').style.backgroundColor = getHeatColor(data.tr);
                document.getElementById('qBL').style.backgroundColor = getHeatColor(data.bl);
                document.getElementById('qBR').style.backgroundColor = getHeatColor(data.br);

                // 3. Atualiza os ângulos na telemetria
                document.getElementById('txtH').innerText = data.angH + '°';
                document.getElementById('txtV').innerText = data.angV + '°';

                // Se estiver no automático, os sliders manuais acompanham o movimento
                if(data.auto) {
                    document.getElementById('rngManH').value = data.angH;
                    document.getElementById('lblManH').innerText = data.angH + '°';
                    document.getElementById('rngManV').value = data.angV;
                    document.getElementById('lblManV').innerText = data.angV + '°';
                }

                // 4. Rotaciona o modelo 3D de acordo com as coordenadas físicas
                const hRot = (data.angH - 90); 
                const vRot = (90 - data.angV); 
                document.getElementById('vPanel').style.transform = `rotateY(${hRot}deg) rotateX(${vRot}deg)`;

            } catch (err) {
                console.error("Erro na leitura de dados: ", err);
            }
        }

        function toggleAuto(isAuto) {
            const card = document.getElementById('manualCard');
            if(isAuto) {
                card.classList.add('disabled');
            } else {
                card.classList.remove('disabled');
            }
            sendControl('auto', isAuto ? 1 : 0);
        }

        function updateTolerance(val) {
            document.getElementById('vTol').innerText = val;
            sendControl('tol', val);
        }

        function moveServo(axis, val) {
            if(axis === 'H') {
                document.getElementById('lblManH').innerText = val + '°';
                sendControl('servoH', val);
            } else {
                document.getElementById('lblManV').innerText = val + '°';
                sendControl('servoV', val);
            }
        }

        function sendControl(param, val) {
            const formData = new FormData();
            formData.append(param, val);
            fetch('/control', { method: 'POST', body: formData });
        }

        window.onload = function() {
            toggleAuto(document.getElementById('chkAuto').checked);
            setInterval(updateTelemetry, 500); // 500ms para evitar travamento do ESP32
        }
    </script>
</body>
</html>
  )rawliteral";

  server.send(200, "text/html", html);
}

// =========================================================
//            RETORNO DOS SENSORIAIS EM JSON
// =========================================================
void handleData() {
  String json = "{";
  json += "\"tl\":" + String(valTL) + ",";
  json += "\"tr\":" + String(valTR) + ",";
  json += "\"bl\":" + String(valBL) + ",";
  json += "\"br\":" + String(valBR) + ",";
  json += "\"angH\":" + String(posHoriz) + ",";
  json += "\"angV\":" + String(posVert) + ",";
  json += "\"auto\":" + String(modoAutomatico ? "true" : "false");
  json += "}";

  server.send(200, "application/json", json);
}

// =========================================================
//                  CONTROLE REMOTO VIA WEB
// =========================================================
void handleControl() {
  if (server.hasArg("auto")) {
    modoAutomatico = (server.arg("auto").toInt() == 1);
    Serial.print("Modo de Operação: ");
    Serial.println(modoAutomatico ? "AUTOMÁTICO" : "MANUAL");
  }

  if (server.hasArg("tol")) {
    tolerancia = server.arg("tol").toInt();
    Serial.print("Tolerância alterada para: ");
    Serial.println(tolerancia);
  }

  if (server.hasArg("servoH") && !modoAutomatico) {
    posHoriz = server.arg("servoH").toInt();
    posHoriz = constrain(posHoriz, limitHorizMin, limitHorizMax);
    servoHorizontal.write(posHoriz);
  }

  if (server.hasArg("servoV") && !modoAutomatico) {
    posVert = server.arg("servoV").toInt();
    posVert = constrain(posVert, limitVertMin, limitVertMax);
    servoVertical.write(posVert);
  }

  server.send(200, "text/plain", "OK");
}