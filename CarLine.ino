#include <WiFi.h>
#include <WebServer.h>
#include <WebSocketsServer.h>
#include <ArduinoJson.h>

#define LOG(x) Serial.println(x)
#define LOGF(...) Serial.printf(__VA_ARGS__)

const char* ssid = "Jendeukie";
const char* password = "thaongan";

WebServer server(80);
WebSocketsServer webSocket(81);

#define S1 26
#define S2 25
#define S3 32
#define S4 33
#define S5 27

#define ENA 21
#define IN1 22
#define IN2 23
#define IN3 18
#define IN4 19
#define ENB 5

#define SPEED 100
#define TURN_SPEED 120

int graph[20][4] = {
  {1,5,-1,-1},{0,6,-1,-1},{3,7,-1,-1},{2,4,8,-1},{3,9,-1,-1},
  {0,6,10,-1},{1,5,7,11},{2,6,8,12},{3,7,9,13},{4,8,14,-1},
  {5,11,15,-1},{6,10,12,16},{7,11,13,17},{8,12,14,18},{9,13,19,-1},
  {10,16,-1,-1},{11,15,-1,-1},{12,18,-1,-1},{13,17,19,-1},{14,18,-1,-1}
};

int edgeWeights[20][20];
int path[50];
int pathLength = 0;
int currentIndex = 0;

bool moving = false;
unsigned long lastNodeTime = 0;

String html = R"rawliteral(
<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>Path Planner & Robot Controller</title>
    <script src="https://cdn.tailwindcss.com"></script>
    <link href="https://cdnjs.cloudflare.com/ajax/libs/font-awesome/6.0.0/css/all.min.css" rel="stylesheet">
    <style>
        @import url('https://fonts.googleapis.com/css2?family=Inter:wght@400;500;600;700&display=swap');
        body { font-family: 'Inter', sans-serif; background-color: #f3f4f6; }
        .glass-panel { background: white; border-radius: 12px; box-shadow: 0 4px 6px -1px rgba(0, 0, 0, 0.1); }
        .node { width: 36px; height: 36px; border-radius: 50%; background: white; border: 3px solid #e5e7eb; display: flex; align-items: center; justify-content: center; font-weight: bold; cursor: pointer; position: absolute; transform: translate(-50%, -50%); transition: all 0.3s; z-index: 10; color: #374151; font-size: 14px;}
        .node:hover { border-color: #6366f1; transform: translate(-50%, -50%) scale(1.1); }
        .node.start { border-color: #ef4444; color: #ef4444; }
        .node.end { border-color: #f59e0b; color: #f59e0b; }
        .node.path { background: #6366f1; color: white; border-color: #4f46e5; }
        .node.current { border-color: #10b981; border-width: 4px; box-shadow: 0 0 0 4px rgba(16, 185, 129, 0.2); }
        .edge { position: absolute; background: #9ca3af; transform-origin: 0 50%; z-index: 5; height: 2px; }
        .edge.path { background: #6366f1; height: 4px; z-index: 6; }
        #canvas { position: relative; width: 100%; height: 100%; min-height: 400px; background-image: radial-gradient(#e5e7eb 1px, transparent 1px); background-size: 20px 20px; border-radius: 8px; overflow: hidden; }
        .scroll-custom::-webkit-scrollbar { width: 6px; }
        .scroll-custom::-webkit-scrollbar-thumb { background-color: #cbd5e1; border-radius: 4px; }
    </style>
</head>
<body class="h-screen flex flex-col overflow-hidden text-gray-800">
    <header class="bg-white border-b border-gray-200 px-6 py-3 flex justify-between items-center z-20 shadow-sm shrink-0">
        <div class="flex items-center gap-3">
            <div class="w-8 h-8 rounded-[8px] bg-indigo-500 flex items-center justify-center text-white"><i class="fa-solid fa-route"></i></div>
            <h1 class="text-xl font-bold text-gray-800">Path Planner & Robot Controller</h1>
            <span class="bg-indigo-50 text-indigo-700 text-[10px] font-bold tracking-wide px-2 py-1 rounded-full ml-2 uppercase">Local</span>
        </div>
        <div class="flex items-center gap-4">
            <div class="flex items-center gap-2 text-xs font-semibold text-gray-600 bg-gray-50 px-3 py-1.5 rounded-full border border-gray-200">
                <div class="w-1.5 h-1.5 rounded-full bg-green-500" id="conn-indicator"></div>
                <span id="conn-status">Connected: ESP32</span>
            </div>
            <button onclick="run()" class="bg-indigo-500 hover:bg-indigo-600 text-white px-5 py-2 rounded-lg text-sm font-semibold shadow-md transition-colors flex items-center gap-2">
                <i class="fa-solid fa-play text-xs"></i> Start Navigation
            </button>
        </div>
    </header>

    <main class="flex-1 flex gap-4 p-4 lg:gap-6 lg:p-6 bg-[#f3f4f6] overflow-hidden">
        <!-- Left Panel: Edges -->
        <div class="w-64 glass-panel flex flex-col overflow-hidden border border-gray-100 flex-shrink-0">
            <div class="px-4 py-3 border-b border-gray-100 bg-gray-50 flex justify-between items-center">
                <h3 class="font-bold text-gray-800 text-sm">Edges & Weights</h3>
                <i class="fa-solid fa-link text-gray-400"></i>
            </div>
            <div class="p-2 border-b border-gray-100 bg-indigo-50 text-xs text-indigo-700 p-3 italic">
                A* algo uses these custom weights to find the shortest path.
            </div>
            <div class="flex-1 overflow-y-auto scroll-custom p-3 space-y-2 h-0" id="edge-list">
                <!-- Edges dynamically populated here -->
            </div>
        </div>

        <!-- Center Panel: Graph Canvas -->
        <div class="flex-1 flex flex-col gap-4 min-w-0">
            <div class="glass-panel p-2 flex-1 relative flex flex-col border border-gray-100">
                <div class="flex justify-between items-center px-4 py-2 border-b border-gray-100">
                    <div class="flex gap-1 text-sm font-bold text-gray-700">Map Layout</div>
                    <div class="text-xs font-semibold text-gray-500 items-center">
                        <label class="flex items-center gap-1"><input type="checkbox" id="show-weights" onchange="drawEdges()" checked class="accent-indigo-500"> Show weights on map</label>
                    </div>
                </div>
                <div id="canvas" class="flex-1 bg-white m-2 border border-gray-100 shadow-inner"></div>
            </div>
            
            <div class="glass-panel p-4 h-40 flex gap-6 border border-gray-100 shrink-0">
                <div class="flex-1 flex flex-col relative w-0">
                    <div class="flex justify-between items-center mb-2">
                        <h3 class="font-bold text-gray-800 text-sm">Event Log</h3>
                        <button onclick="document.getElementById('log').innerHTML=''" class="text-[10px] text-gray-500 hover:text-gray-700 bg-gray-50 border rounded px-1.5 py-1">Clear</button>
                    </div>
                    <div id="log" class="flex-1 overflow-y-auto scroll-custom text-xs font-mono text-gray-600 space-y-1.5 pr-2"></div>
                </div>
            </div>
        </div>

        <!-- Right Panel: Controls -->
        <div class="w-80 flex flex-col gap-4 overflow-y-auto scroll-custom pb-2 flex-shrink-0">
            <div class="glass-panel p-4 border border-gray-100">
                <h3 class="font-bold text-gray-800 mb-3 text-sm">Algorithm</h3>
                <div class="relative mb-4">
                    <select id="algo" class="w-full appearance-none border border-gray-300 bg-gray-50 px-3 py-2 rounded-lg text-sm focus:outline-none focus:ring-2 focus:ring-indigo-500 font-semibold text-indigo-700">
                        <option value="astar">A* (Optimal Path & Weights)</option>
                        <option value="bfs">Breadth-First Search (No Weights)</option>
                        <option value="dfs">Depth-First Search (No Weights)</option>
                    </select>
                    <div class="pointer-events-none absolute inset-y-0 right-0 flex items-center px-3 text-indigo-500"><i class="fa-solid fa-chevron-down text-xs"></i></div>
                </div>

                <div class="flex gap-2 mb-4">
                    <div class="flex-1">
                        <label class="block text-[10px] font-bold text-gray-500 mb-1 uppercase tracking-wide">Start Node</label>
                        <div class="flex items-center gap-1.5 border border-gray-200 rounded-lg px-2 py-1.5 bg-white">
                            <div class="w-4 h-4 rounded-full bg-red-500 flex items-center justify-center text-[8px] font-bold text-white">S</div>
                            <span id="start-label" class="text-sm font-semibold text-gray-700">--</span>
                        </div>
                    </div>
                    <div class="flex-1">
                        <label class="block text-[10px] font-bold text-gray-500 mb-1 uppercase tracking-wide">End Node</label>
                        <div class="flex items-center gap-1.5 border border-gray-200 rounded-lg px-2 py-1.5 bg-white">
                            <div class="w-4 h-4 rounded-full bg-yellow-500 flex items-center justify-center text-[8px] font-bold text-white">E</div>
                            <span id="end-label" class="text-sm font-semibold text-gray-700">--</span>
                        </div>
                    </div>
                </div>

                <button onclick="run()" class="w-full bg-indigo-500 hover:bg-indigo-600 text-white py-2 rounded-lg text-sm font-semibold shadow-md transition-colors flex justify-center items-center gap-2">
                    <i class="fa-solid fa-wand-magic-sparkles text-xs"></i> Find Path
                </button>
            </div>

            <div class="glass-panel p-4 border border-gray-100">
                <div class="flex justify-between items-center mb-2">
                    <h3 class="font-bold text-gray-800 text-sm">Result</h3>
                </div>
                <div class="bg-gray-50 border border-gray-100 rounded-lg p-2.5 mb-3 text-center flex items-center justify-center">
                    <span id="path-result" class="text-xs font-semibold text-gray-500">Path will appear here</span>
                </div>
                <div class="flex gap-2">
                    <div class="flex-1 bg-white rounded-lg py-1.5 text-center border border-gray-100 shadow-sm">
                        <p class="text-[9px] text-gray-500 font-bold uppercase tracking-wide">Total Cost</p>
                        <p id="cost-val" class="text-base font-bold text-indigo-700">-</p>
                    </div>
                    <div class="flex-1 bg-white rounded-lg py-1.5 text-center border border-gray-100 shadow-sm">
                        <p class="text-[9px] text-gray-500 font-bold uppercase tracking-wide">Nodes</p>
                        <p id="nodes-val" class="text-base font-bold text-gray-800">-</p>
                    </div>
                </div>
            </div>

            <div class="glass-panel p-4 relative border border-gray-100">
                <div class="flex justify-between items-center mb-3">
                    <h3 class="font-bold text-gray-800 text-sm">Robot Status</h3>
                    <span id="robot-badge" class="text-[9px] font-bold tracking-widest text-gray-600 bg-gray-100 px-1.5 py-0.5 rounded uppercase">Idle</span>
                </div>
                <div class="flex items-center gap-3 mb-3">
                    <div class="w-10 h-10 bg-indigo-50 rounded-full flex items-center justify-center border-2 border-indigo-100 relative shadow-sm">
                        <i class="fa-solid fa-robot text-lg text-indigo-500"></i>
                    </div>
                    <div class="flex-1 text-xs font-medium space-y-1">
                        <div class="flex justify-between items-center">
                            <span class="text-gray-500">Node:</span>
                            <span id="cur-node" class="font-bold bg-green-500 text-white w-5 h-5 flex items-center justify-center rounded-full text-[10px] shadow-sm">-</span>
                        </div>
                        <div class="flex justify-between items-center">
                            <span class="text-gray-500">Next:</span>
                            <span id="nxt-node" class="font-bold text-indigo-600">-</span>
                        </div>
                    </div>
                </div>
                <div class="flex items-center gap-2">
                    <div class="flex-1 bg-gray-200 rounded-full h-2 overflow-hidden">
                        <div id="progress-bar" class="bg-indigo-500 h-2 rounded-full transition-all duration-300" style="width: 0%"></div>
                    </div>
                    <div class="text-[9px] text-gray-500 font-bold w-6 text-right" id="progress-text">0%</div>
                </div>
            </div>
            
            <button onclick="location.reload()" class="w-full text-center py-2 text-[11px] font-bold text-gray-400 hover:text-gray-600 transition-colors uppercase tracking-widest bg-white border rounded">
                Reset Interface
            </button>
        </div>
    </main>

    <script>
        const ws = new WebSocket("ws://"+location.hostname+":81");
        ws.onopen = () => { logMsg('Connected to ESP32 Graph Engine'); };
        ws.onclose = () => { logMsg('<span class="text-red-500">Connection dropped</span>'); document.getElementById('conn-indicator').className='w-1.5 h-1.5 rounded-full bg-red-500'; document.getElementById('conn-status').innerText='Offline'; };

        const canvas = document.getElementById('canvas');
        let start = null, end = null, finalPath = [];

        const cols = 5, rows = 4;
        const defaultEdges = [
            [0,1],[0,5],[1,6],[2,3],[2,7],[3,4],[3,8],[4,9],
            [5,6],[5,10],[6,7],[6,11],[7,8],[7,12],[8,9],[8,13],
            [9,14],[10,11],[10,15],[11,12],[11,16],[12,13],[12,17],
            [13,14],[13,18],[14,19],[15,16],[16,17],[17,18],[18,19]
        ];
        
        // Edge objects arrays
        let edgeData = defaultEdges.map((e, idx) => ({ u: e[0], v: e[1], w: 1, id: idx }));

        function initSidebar() {
            const list = document.getElementById('edge-list');
            list.innerHTML = '';
            edgeData.forEach(e => {
                const item = document.createElement('div');
                item.className = 'flex justify-between items-center bg-white p-2 rounded border border-gray-100 shadow-sm';
                item.innerHTML = `
                    <div class="flex items-center gap-1.5 text-xs text-gray-700 font-semibold">
                        <div class="w-4 h-4 bg-gray-200 rounded-full flex items-center justify-center text-[9px]">${e.u}</div>
                        <div class="w-3 h-0.5 bg-gray-300"></div>
                        <div class="w-4 h-4 bg-gray-200 rounded-full flex items-center justify-center text-[9px]">${e.v}</div>
                    </div>
                    <div class="flex items-center gap-1">
                        <input type="number" min="1" max="99" value="${e.w}" class="w-12 border border-gray-300 rounded px-1.5 py-0.5 text-xs text-center font-bold text-indigo-600 focus:outline-none focus:border-indigo-500" onchange="updateWeight(${e.id}, this.value)">
                    </div>
                `;
                list.appendChild(item);
            });
        }

        function updateWeight(id, val) {
            let v = parseInt(val);
            if(isNaN(v) || v < 1) v = 1;
            edgeData[id].w = v;
            drawEdges();
        }

        function getPos(id) {
            const w = canvas.clientWidth, h = canvas.clientHeight;
            return {x: (id % cols + 1) * (w / (cols + 1)), y: (Math.floor(id / cols) + 1) * (h / (rows + 1))};
        }

        function drawEdges() {
            document.querySelectorAll('.edge-container').forEach(e => e.remove());
            const showWeights = document.getElementById('show-weights').checked;
            
            edgeData.forEach(edge => {
                const p1 = getPos(edge.u), p2 = getPos(edge.v);
                
                // Wrap edge line and weight in a container
                const container = document.createElement('div');
                container.className = 'edge-container';
                
                const div = document.createElement('div');
                div.className = 'edge'; div.id = 'e_'+edge.u+'_'+edge.v;
                div.style.width = Math.hypot(p2.x - p1.x, p2.y - p1.y) + 'px';
                div.style.left = p1.x + 'px'; div.style.top = p1.y + 'px';
                div.style.transform = `translateY(-50%) rotate(${Math.atan2(p2.y - p1.y, p2.x - p1.x)}rad)`;
                container.appendChild(div);
                
                if (showWeights && edge.w !== 1) {
                    const lbl = document.createElement('div');
                    lbl.className = 'absolute bg-white px-1 shadow-sm text-[10px] font-bold text-indigo-500 rounded border border-indigo-100 z-10 edit-weight';
                    lbl.innerText = edge.w;
                    lbl.style.left = (p1.x + p2.x)/2 + 'px'; lbl.style.top = (p1.y + p2.y)/2 + 'px';
                    lbl.style.transform = 'translate(-50%, -50%)';
                    container.appendChild(lbl);
                }
                
                canvas.appendChild(container);
            });
            if(finalPath.length) highlightPath(finalPath);
        }

        function drawNodes() {
            document.querySelectorAll('.node').forEach(e => e.remove());
            for(let i=0; i<20; i++) {
                const pos = getPos(i);
                const d = document.createElement('div');
                d.className = 'node'; d.id = 'n' + i; d.innerText = i;
                d.style.left = pos.x + 'px'; d.style.top = pos.y + 'px';
                d.onclick = () => select(i);
                canvas.appendChild(d);
            }
        }

        window.onresize = () => { drawEdges(); drawNodes(); };
        setTimeout(() => { initSidebar(); drawEdges(); drawNodes(); }, 100);

        function logMsg(msg) {
            const l = document.getElementById('log');
            l.innerHTML += `<div class="py-0.5"><span class="text-indigo-400 mr-2">[${new Date().toLocaleTimeString('en-US',{hour12:false})}]</span> ${msg}</div>`;
            l.scrollTop = l.scrollHeight;
        }

        function select(i) {
            if (start === null) {
                start = i; document.getElementById('n'+i).classList.add('start'); document.getElementById('start-label').innerText = i;
            } else if (end === null && i !== start) {
                end = i; document.getElementById('n'+i).classList.add('end'); document.getElementById('end-label').innerText = i;
            } else if (i === start) {
                start = null; document.getElementById('n'+i).classList.remove('start'); document.getElementById('start-label').innerText = '--';
            } else if (i === end) {
                end = null; document.getElementById('n'+i).classList.remove('end'); document.getElementById('end-label').innerText = '--';
            }
        }

        function highlightPath(p) {
            document.querySelectorAll('.node').forEach(n => n.classList.remove('path'));
            document.querySelectorAll('.edge').forEach(e => e.classList.remove('path'));
            p.forEach(id => { let el = document.getElementById('n'+id); if(el) el.classList.add('path'); });
            for(let i=0; i<p.length-1; i++){
                let u=Math.min(p[i],p[i+1]), v=Math.max(p[i],p[i+1]);
                let el = document.getElementById('e_'+u+'_'+v);
                if(el) el.classList.add('path');
            }
        }

        function run() {
            if (start === null || end === null) return alert("Select Start & End node on canvas first!");
            let algo = document.getElementById('algo').value;
            
            // Only send modified weights to avoid huge payloads
            let weightPayload = edgeData.filter(e => e.w !== 1).map(e => ({u: e.u, v: e.v, w: e.w}));
            
            ws.send(JSON.stringify({start, end, algo: algo, w: weightPayload}));
            
            document.getElementById('robot-badge').innerText = 'Navigating';
            document.getElementById('robot-badge').className = 'text-[9px] font-bold tracking-widest text-indigo-700 bg-indigo-100 px-1.5 py-0.5 rounded uppercase';
            document.getElementById('progress-bar').style.width = '0%';
            logMsg(`Calculating... Payload Sent (Weights: ${weightPayload.length})`);
        }

        ws.onmessage = (msg) => {
            let data = JSON.parse(msg.data);
            if (data.path) {
                finalPath = data.path; highlightPath(finalPath);
                document.getElementById('path-result').innerHTML = finalPath.length ? '<div class="text-indigo-600 font-bold bg-indigo-50 px-3 py-1.5 rounded-full">' + finalPath.join(' <i class="fa-solid fa-arrow-right text-[10px] opacity-50 mx-0.5"></i> ') + '</div>' : 'No path';
                
                // compute total custom cost
                let costSum = 0;
                for(let i=0; i<finalPath.length-1; i++) {
                    let u=finalPath[i], v=finalPath[i+1];
                    let match = edgeData.find(e => (e.u==u && e.v==v) || (e.u==v && e.v==u));
                    costSum += match ? match.w : 1;
                }
                
                document.getElementById('cost-val').innerText = finalPath.length ? costSum : 0;
                document.getElementById('nodes-val').innerText = finalPath.length;
                logMsg(`Path compiled. Moves: ${finalPath.length-1}`);
            }
            if (data.node !== undefined) {
                document.querySelectorAll('.node').forEach(n => n.classList.remove('current'));
                let el = document.getElementById('n'+data.node); if(el) el.classList.add('current');
                document.getElementById('cur-node').innerText = data.node;
                let idx = finalPath.indexOf(data.node);
                if (idx >= 0 && idx < finalPath.length - 1) {
                    document.getElementById('nxt-node').innerText = finalPath[idx + 1];
                    let pct = Math.round((idx / (finalPath.length - 1)) * 100);
                    document.getElementById('progress-bar').style.width = pct + '%';
                    document.getElementById('progress-text').innerText = pct + '%';
                }
            }
            if (data.log) {
                if (data.log === "FINISH") {
                    document.getElementById('robot-badge').innerText = 'Arrived';
                    document.getElementById('robot-badge').className = 'text-[9px] font-bold tracking-widest text-white bg-green-500 px-1.5 py-0.5 rounded uppercase';
                    document.getElementById('progress-bar').style.width = '100%';
                    document.getElementById('nxt-node').innerText = '-';
                    logMsg('Navigation complete.');
                }
            }
        };
    </script>
</body>
</html>
)rawliteral";

void sendNode(int n){
  JsonDocument doc;
  doc["node"] = n;
  String msg;
  serializeJson(doc, msg);
  webSocket.broadcastTXT(msg);
}

void sendLog(String t){
  JsonDocument doc;
  doc["log"] = t;
  String msg;
  serializeJson(doc, msg);
  webSocket.broadcastTXT(msg);
}

void setMotor(int L, int R){
  digitalWrite(IN3, L > 0 ? LOW : HIGH);
  digitalWrite(IN4, L > 0 ? HIGH : LOW);
  digitalWrite(IN1, R > 0 ? LOW : HIGH);
  digitalWrite(IN2, R > 0 ? HIGH : LOW);

  ledcWrite(ENA, abs(R));
  ledcWrite(ENB, abs(L));
}

void stopMotor(){
  ledcWrite(ENA, 0);
  ledcWrite(ENB, 0);

  digitalWrite(IN1, LOW);
  digitalWrite(IN2, LOW);
  digitalWrite(IN3, LOW);
  digitalWrite(IN4, LOW);
}

int readLine(){
  if(digitalRead(S3)==1) return 0;
  if(digitalRead(S2)==1) return -1;
  if(digitalRead(S1)==1) return -2;
  if(digitalRead(S4)==1) return 1;
  if(digitalRead(S5)==1) return 2;
  return 99;
}

void followLine(){
  int p = readLine();

  if(p == 0) setMotor(SPEED, SPEED);
  else if(p < 0) setMotor(SPEED - 80, SPEED);
  else if(p > 0) setMotor(SPEED, SPEED - 80);
  else stopMotor();
}


bool isNode(){
  // Chống lỗi đếm Node trên đường thẳng (do băng dính dày làm xe lắc lên 3 mắt giữa)
  // Tính tổng số mắt đang đè lên vạch đen
  int sum = digitalRead(S1) + digitalRead(S2) + digitalRead(S3) + digitalRead(S4) + digitalRead(S5);
  
  // Mắt rìa ngoài cùng (S1 hoặc S5) phải thấy vạch ngang, và mắt giữa S3 cũng phải thấy
  bool hasOuter = (digitalRead(S1) == 1 || digitalRead(S5) == 1);
  bool hasCenter = (digitalRead(S3) == 1);

  // Điều kiện để là 1 ngã tư chuẩn:
  // 1. Quét trúng ít nhất 4 mắt cùng lúc (vạch ngang rộng)
  // 2. HOẶC mắt ngoài cùng và mắt trong cùng đều chạm => Chắc chắn là vạch ngang ngã tư chạy cắt qua!
  return (sum >= 4) || (hasOuter && hasCenter);
}

void bfs(int s,int e){
  bool vis[20]={0};
  int parent[20];
  for(int i=0;i<20;i++) parent[i]=-1;

  int q[50],f=0,r=0;
  q[r++]=s;
  vis[s]=1;

  while(f<r){
    int u=q[f++];
    if(u==e) break;

    for(int i=0;i<4;i++){
      int v=graph[u][i];
      if(v!=-1 && !vis[v]){
        vis[v]=1;
        parent[v]=u;
        q[r++]=v;
      }
    }
  }

  pathLength=0;
  for(int v=e;v!=-1;v=parent[v]){
    path[pathLength++]=v;
  }

  for(int i=0;i<pathLength/2;i++){
    int t=path[i];
    path[i]=path[pathLength-1-i];
    path[pathLength-1-i]=t;
  }
}

bool dfsUtil(int u,int e,bool vis[],int parent[]){
  vis[u]=1;
  if(u==e) return true;

  for(int i=0;i<4;i++){
    int v=graph[u][i];
    if(v!=-1 && !vis[v]){
      parent[v]=u;
      if(dfsUtil(v,e,vis,parent)) return true;
    }
  }
  return false;
}

void dfs(int s,int e){
  bool vis[20]={0};
  int parent[20];
  for(int i=0;i<20;i++) parent[i]=-1;

  dfsUtil(s,e,vis,parent);

  pathLength=0;
  for(int v=e;v!=-1;v=parent[v]){
    path[pathLength++]=v;
  }

  for(int i=0;i<pathLength/2;i++){
    int t=path[i];
    path[i]=path[pathLength-1-i];
    path[pathLength-1-i]=t;
  }
}

int h(int a,int b){
  return abs(a-b);
}

void astar(int s,int e){
  bool vis[20]={0};
  int parent[20],g[20];

  for(int i=0;i<20;i++){
    parent[i]=-1;
    g[i]=9999;
  }

  g[s]=0;

  for(int k=0;k<20;k++){
    int u=-1,best=9999;

    for(int i=0;i<20;i++){
      if(!vis[i] && g[i]+h(i,e)<best){
        best=g[i]+h(i,e);
        u=i;
      }
    }

    if(u==-1) break;

    vis[u]=1;

    for(int i=0;i<4;i++){
      int v=graph[u][i];
      if(v!=-1 && g[u] + edgeWeights[u][v] < g[v]){
        g[v]=g[u]+edgeWeights[u][v];
        parent[v]=u;
      }
    }
  }

  pathLength=0;
  for(int v=e;v!=-1;v=parent[v]){
    path[pathLength++]=v;
  }

  for(int i=0;i<pathLength/2;i++){
    int t=path[i];
    path[i]=path[pathLength-1-i];
    path[pathLength-1-i]=t;
  }
}

String getDir(int a,int b){
  if(b==a+5) return "DOWN";
  if(b==a-5) return "UP";
  if(b==a+1) return "RIGHT";
  if(b==a-1) return "LEFT";
  return "";
}

void turnLeft(){
  setMotor(-TURN_SPEED, TURN_SPEED);
  
  // 1. Xoay mù 300ms để S3 lệch khỏi vạch ngã tư hiện tại
  unsigned long t_blind = millis();
  while(millis() - t_blind < 300) { server.handleClient(); webSocket.loop(); }

  // 2. Tiếp tục xoay tìm vạch mới
  unsigned long startTime = millis();
  while(digitalRead(S3)==0){
    webSocket.loop();
    server.handleClient();
    if(millis() - startTime > 1500) break; // timeout
  }

  stopMotor();
  delay(100);
}

void turnRight(){
  setMotor(TURN_SPEED, -TURN_SPEED);

  // 1. Xoay mù 300ms để S3 lệch khỏi vạch ngã tư hiện tại
  unsigned long t_blind = millis();
  while(millis() - t_blind < 300) { server.handleClient(); webSocket.loop(); }

  // 2. Tiếp tục xoay tìm vạch mới
  unsigned long startTime = millis();
  while(digitalRead(S3)==0){
    webSocket.loop();
    server.handleClient();
    if(millis() - startTime > 1500) break;
  }

  stopMotor();
  delay(100);
}

void onWS(uint8_t n,WStype_t t,uint8_t *p,size_t l){
  if(t==WStype_TEXT){
    JsonDocument doc;
    deserializeJson(doc,p);

    int s=doc["start"];
    int e=doc["end"];
    String algo=doc["algo"];

    // Reset default weights
    for(int i=0; i<20; i++) for(int j=0; j<20; j++) edgeWeights[i][j] = 1;
    
    // Parse custom weights sent from web
    if(doc.containsKey("w")) {
      JsonArray wArray = doc["w"].as<JsonArray>();
      for(JsonVariant edge : wArray) {
        int u = edge["u"]; int v = edge["v"]; int w = edge["w"];
        edgeWeights[u][v] = w; edgeWeights[v][u] = w;
      }
    }

    if(algo=="bfs") bfs(s,e);
    else if(algo=="dfs") dfs(s,e);
    else astar(s,e);

    JsonDocument res;
    JsonArray arr=res["path"].to<JsonArray>();

    for(int i=0;i<pathLength;i++){
      arr.add(path[i]);
    }

    String out;
    serializeJson(res,out);
    webSocket.broadcastTXT(out);

    currentIndex = 0;
    moving = true;
    lastNodeTime = 0;

    sendLog("START");
  }
}

void setup(){
  for(int i=0; i<20; i++){
    for(int j=0; j<20; j++) edgeWeights[i][j] = 1;
  }

  Serial.begin(115200);

  pinMode(S1,INPUT);
  pinMode(S2,INPUT);
  pinMode(S3,INPUT);
  pinMode(S4,INPUT);
  pinMode(S5,INPUT);

  pinMode(IN1,OUTPUT);
  pinMode(IN2,OUTPUT);
  pinMode(IN3,OUTPUT);
  pinMode(IN4,OUTPUT);

  pinMode(ENA,OUTPUT);
  pinMode(ENB,OUTPUT);

  ledcAttach(ENA,1000,8);
  ledcAttach(ENB,1000,8);

  WiFi.begin(ssid,password);

  while(WiFi.status()!=WL_CONNECTED){
    delay(500);
    Serial.print(".");
  }

  LOG("\nWiFi OK");
  LOG(WiFi.localIP());

  server.on("/",[](){
    server.send(200,"text/html",html);
  });

  server.begin();

  webSocket.begin();
  webSocket.onEvent(onWS);
}

void loop(){
  server.handleClient();
  webSocket.loop();

  if(moving && pathLength > 1){
    followLine();

    // Tăng thời gian cooldown giữa 2 node liên tiếp lên 1200ms để chống dội (bouncing)
    if(isNode() && millis() - lastNodeTime > 1200){

      int from = path[currentIndex];
      int to = path[currentIndex + 1];

      String dir = getDir(from,to);

      sendNode(to);

      if(dir=="LEFT"){
        turnLeft();
      }
      else if(dir=="RIGHT"){
        turnRight();
      }
      else {
        // Đi thẳng: lao mù trong 200ms để chắc chắn đuôi xe thoát khỏi vạch ngang ngã tư
        setMotor(SPEED, SPEED);
        unsigned long t_blind = millis();
        while(millis() - t_blind < 200) { server.handleClient(); webSocket.loop(); }
      }

      currentIndex++;
      
      // Update lại lastNodeTime SAU KHI đã rẽ hoặc qua ngã tư xong
      lastNodeTime = millis();

      if(currentIndex >= pathLength - 1){
        stopMotor();
        moving = false;
        sendLog("FINISH");
        pathLength = 0;
      }
    }
  }
}