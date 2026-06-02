import re

new_html = r"""<!DOCTYPE html>
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
        .glass-panel { background: white; border-radius: 12px; box-shadow: 0 4px 6px -1px rgba(0, 0, 0, 0.1), 0 2px 4px -1px rgba(0, 0, 0, 0.06); }
        .node { width: 40px; height: 40px; border-radius: 50%; background: white; border: 3px solid #e5e7eb; display: flex; align-items: center; justify-content: center; font-weight: bold; cursor: pointer; position: absolute; transform: translate(-50%, -50%); transition: all 0.3s; z-index: 10; color: #374151; box-shadow: 0 2px 4px rgba(0,0,0,0.1); }
        .node:hover { border-color: #6366f1; transform: translate(-50%, -50%) scale(1.1); }
        .node.start { border-color: #ef4444; color: #ef4444; }
        .node.end { border-color: #f59e0b; color: #f59e0b; }
        .node.path { background: #6366f1; color: white; border-color: #4f46e5; }
        .node.current { border-color: #10b981; border-width: 4px; box-shadow: 0 0 0 4px rgba(16, 185, 129, 0.2); }
        .edge { position: absolute; background: #9ca3af; transform-origin: 0 50%; z-index: 5; height: 2px; }
        .edge.path { background: #6366f1; height: 4px; z-index: 6; }
        #canvas { position: relative; width: 100%; height: 500px; background-image: radial-gradient(#e5e7eb 1px, transparent 1px); background-size: 20px 20px; border-radius: 8px; overflow: hidden; }
        .scroll-log::-webkit-scrollbar { width: 6px; }
        .scroll-log::-webkit-scrollbar-thumb { background-color: #d1d5db; border-radius: 4px; }
    </style>
</head>
<body class="h-screen flex flex-col overflow-hidden text-gray-800">
    <header class="bg-white border-b border-gray-200 px-6 py-3 flex justify-between items-center z-20 shadow-sm">
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
            <button class="text-gray-400 hover:text-gray-700"><i class="fa-solid fa-cog"></i></button>
            <button onclick="run()" class="bg-indigo-500 hover:bg-indigo-600 text-white px-5 py-2 rounded-lg text-sm font-semibold shadow-md shadow-indigo-200 transition-colors flex items-center gap-2">
                <i class="fa-solid fa-play text-xs"></i> Start Navigation
            </button>
        </div>
    </header>

    <main class="flex-1 flex gap-6 p-6 bg-[#f3f4f6] overflow-hidden">
        <div class="flex-1 flex flex-col gap-4">
            <div class="glass-panel p-2 flex-1 relative flex flex-col">
                <div class="flex justify-between items-center px-4 py-2 border-b border-gray-100">
                    <div class="flex gap-1">
                        <button class="p-2 w-8 h-8 flex items-center justify-center text-indigo-600 bg-indigo-50 rounded hover:bg-indigo-100"><i class="fa-solid fa-arrow-pointer text-sm"></i></button>
                        <button class="p-2 w-8 h-8 flex items-center justify-center text-gray-400 hover:bg-gray-50 rounded"><i class="fa-regular fa-hand text-sm"></i></button>
                        <button class="p-2 w-8 h-8 flex items-center justify-center text-gray-400 hover:bg-gray-50 rounded"><i class="fa-solid fa-pen text-sm"></i></button>
                    </div>
                    <div class="flex gap-3 text-xs font-semibold text-gray-500 items-center">
                        <label class="flex items-center gap-1 cursor-pointer"><input type="checkbox" checked class="accent-indigo-500"> Show weights</label>
                        <button class="flex items-center gap-1 hover:text-gray-800 bg-gray-50 px-2 py-1 rounded border"><i class="fa-solid fa-border-all"></i> Grid</button>
                    </div>
                </div>
                <div id="canvas" class="flex-1 bg-white m-4 border border-gray-100 shadow-inner"></div>
                
                <!-- Floating Toolbar Zoom -->
                <div class="absolute left-6 bottom-6 flex flex-col bg-white border border-gray-200 rounded-lg shadow-sm text-gray-500 overflow-hidden">
                    <button class="p-2 hover:bg-gray-50 border-b"><i class="fa-solid fa-magnifying-glass-plus"></i></button>
                    <button class="p-2 hover:bg-gray-50 border-b"><i class="fa-solid fa-magnifying-glass-minus"></i></button>
                    <button class="p-2 hover:bg-gray-50"><i class="fa-solid fa-expand"></i></button>
                </div>
            </div>
            
            <div class="glass-panel p-4 h-40 flex gap-6">
                <div class="flex-1 flex flex-col relative">
                    <div class="flex justify-between items-center mb-2">
                        <h3 class="font-bold text-gray-800 text-sm">Event Log</h3>
                        <button onclick="document.getElementById('log').innerHTML=''" class="text-xs text-gray-500 hover:text-gray-700 bg-gray-50 border rounded px-2 py-1"><i class="fa-solid fa-trash mr-1"></i>Clear</button>
                    </div>
                    <div id="log" class="flex-1 overflow-y-auto scroll-log text-xs font-mono text-gray-600 space-y-1.5 pr-2"></div>
                </div>
                <!-- Legend -->
                <div class="w-48 bg-gray-50 rounded border border-gray-100 p-3 flex flex-col justify-center gap-2 text-xs font-medium text-gray-600">
                    <h4 class="font-bold text-gray-800 mb-1">Legend</h4>
                    <div class="flex gap-4">
                        <div class="flex items-center gap-1"><div class="w-3 h-3 rounded-full border-2 border-red-500"></div> Start</div>
                        <div class="flex items-center gap-1"><div class="w-3 h-3 rounded-full border-2 border-yellow-500"></div> End</div>
                    </div>
                    <div class="flex gap-4 mt-1">
                        <div class="flex items-center gap-1"><div class="w-4 h-1 bg-indigo-500 rounded"></div> Path</div>
                        <div class="flex items-center gap-1"><div class="w-3 h-3 rounded-full border-2 border-green-500 bg-green-100"></div> Current</div>
                    </div>
                </div>
            </div>
        </div>

        <div class="w-80 flex flex-col gap-4 overflow-y-auto pb-4 hide-scrollbar">
            <div class="glass-panel p-5">
                <h3 class="font-bold text-gray-800 mb-3 text-sm">Algorithm</h3>
                <div class="relative mb-5">
                    <select id="algo" class="w-full appearance-none border border-gray-300 bg-white px-3 py-2 rounded-lg text-sm focus:outline-none focus:ring-2 focus:ring-indigo-500 font-medium text-gray-700">
                        <option value="astar">A* (Optimal Path)</option>
                        <option value="dijkstra">Dijkstra (Shortest Path)</option>
                        <option value="bfs">Breadth-First Search</option>
                        <option value="dfs">Depth-First Search</option>
                    </select>
                    <div class="pointer-events-none absolute inset-y-0 right-0 flex items-center px-3 text-gray-400">
                        <i class="fa-solid fa-chevron-down text-xs"></i>
                    </div>
                </div>

                <div class="flex gap-2 mb-5">
                    <div class="flex-1">
                        <label class="block text-[10px] font-bold text-gray-500 mb-1 uppercase tracking-wide">Start Node</label>
                        <div class="flex items-center justify-between border border-gray-300 rounded-lg px-2 py-1.5 bg-white">
                            <div class="flex items-center gap-1.5">
                                <div class="w-4 h-4 rounded-full bg-red-500 flex items-center justify-center text-[8px] font-bold text-white">S</div>
                                <span id="start-label" class="text-sm font-semibold text-gray-700">--</span>
                            </div>
                            <i class="fa-solid fa-chevron-down text-[10px] text-gray-400"></i>
                        </div>
                    </div>
                    <div class="flex-1">
                        <label class="block text-[10px] font-bold text-gray-500 mb-1 uppercase tracking-wide">End Node</label>
                        <div class="flex items-center justify-between border border-gray-300 rounded-lg px-2 py-1.5 bg-white">
                            <div class="flex items-center gap-1.5">
                                <div class="w-4 h-4 rounded-full bg-yellow-500 flex items-center justify-center text-[8px] font-bold text-white">E</div>
                                <span id="end-label" class="text-sm font-semibold text-gray-700">--</span>
                            </div>
                            <i class="fa-solid fa-chevron-down text-[10px] text-gray-400"></i>
                        </div>
                    </div>
                </div>

                <button onclick="run()" class="w-full bg-indigo-500 hover:bg-indigo-600 text-white py-2 rounded-lg text-sm font-semibold shadow-md transition-colors flex justify-center items-center gap-2">
                    <i class="fa-solid fa-wand-magic-sparkles text-xs"></i> Find Path
                </button>
            </div>

            <div class="glass-panel p-5">
                <div class="flex justify-between items-center mb-3">
                    <h3 class="font-bold text-gray-800 text-sm">Result</h3>
                    <span class="text-[9px] font-bold tracking-wide text-teal-600 bg-teal-50 px-1.5 py-0.5 rounded uppercase">Optimal</span>
                </div>
                <div class="bg-gray-50 border border-gray-100 rounded-lg p-3 mb-3 text-center min-h-[50px] flex items-center justify-center">
                    <span id="path-result" class="text-xs font-semibold text-gray-500">Path will appear here</span>
                </div>
                <div class="flex gap-2">
                    <div class="flex-1 bg-white rounded-lg py-2 text-center border border-gray-100 shadow-sm">
                        <p class="text-[10px] text-gray-500 font-bold uppercase tracking-wide mb-0.5">Total Cost</p>
                        <p id="cost-val" class="text-lg font-bold text-gray-800">-</p>
                    </div>
                    <div class="flex-1 bg-white rounded-lg py-2 text-center border border-gray-100 shadow-sm">
                        <p class="text-[10px] text-gray-500 font-bold uppercase tracking-wide mb-0.5">Nodes</p>
                        <p id="nodes-val" class="text-lg font-bold text-gray-800">-</p>
                    </div>
                </div>
            </div>

            <div class="glass-panel p-5 relative border border-gray-100 shadow-sm">
                <div class="flex justify-between items-center mb-3">
                    <h3 class="font-bold text-gray-800 text-sm">Robot Status</h3>
                    <span id="robot-badge" class="text-[9px] font-bold tracking-widest text-green-700 bg-green-100 px-1.5 py-0.5 rounded uppercase">Running</span>
                </div>
                
                <div class="flex items-center gap-4 mb-4">
                    <div class="w-12 h-12 bg-indigo-50 rounded-full flex items-center justify-center border-2 border-indigo-100 relative shadow-sm">
                        <i class="fa-solid fa-robot text-xl text-indigo-500"></i>
                        <span class="absolute top-0 right-0 w-2.5 h-2.5 bg-green-500 border-2 border-white rounded-full"></span>
                    </div>
                    <div class="flex-1 space-y-1.5 text-xs font-medium">
                        <div class="flex justify-between items-center">
                            <span class="text-gray-500">Current Node:</span>
                            <span id="cur-node" class="font-bold bg-green-500 text-white w-5 h-5 flex items-center justify-center rounded-full text-[10px] shadow-sm shadow-green-200">-</span>
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
                    <div class="text-[10px] text-gray-500 font-bold w-6 text-right" id="progress-text">0%</div>
                </div>
            </div>
            
            <button onclick="location.reload()" class="w-full text-center py-2 text-xs font-bold text-gray-400 hover:text-gray-600 transition-colors uppercase tracking-widest">
                Reset Interface
            </button>
        </div>
    </main>

    <script>
        const ws = new WebSocket("ws://"+location.hostname+":81");
        
        ws.onopen = () => { logMsg('<span class="text-green-500"><i class="fa-solid fa-check"></i> Connected to ESP32 Graph Engine</span>'); };
        ws.onclose = () => { logMsg('<span class="text-red-500"><i class="fa-solid fa-xmark"></i> Connection dropped</span>'); document.getElementById('conn-indicator').className='w-1.5 h-1.5 rounded-full bg-red-500'; document.getElementById('conn-status').innerText='Offline'; };

        const canvas = document.getElementById('canvas');
        let start = null, end = null, finalPath = [];

        const cols = 5, rows = 4;
        const edges = [
            [0,1], [0,5], [1,6], [2,3], [2,7], [3,4], [3,8], [4,9],
            [5,6], [5,10], [6,7], [6,11], [7,8], [7,12], [8,9], [8,13],
            [9,14], [10,11], [10,15], [11,12], [11,16], [12,13], [12,17],
            [13,14], [13,18], [14,19], [15,16], [16,17], [17,18], [18,19]
        ];

        function getPos(id) {
            const w = canvas.clientWidth, h = canvas.clientHeight;
            return {x: (id % cols + 1) * (w / (cols + 1)), y: (Math.floor(id / cols) + 1) * (h / (rows + 1))};
        }

        function drawEdges() {
            document.querySelectorAll('.edge').forEach(e => e.remove());
            edges.forEach(edge => {
                const p1 = getPos(edge[0]), p2 = getPos(edge[1]);
                const div = document.createElement('div');
                div.className = 'edge'; div.id = 'e_'+edge[0]+'_'+edge[1];
                div.style.width = Math.hypot(p2.x - p1.x, p2.y - p1.y) + 'px';
                div.style.left = p1.x + 'px'; div.style.top = p1.y + 'px';
                div.style.transform = `translateY(-50%) rotate(${Math.atan2(p2.y - p1.y, p2.x - p1.x)}rad)`;
                canvas.appendChild(div);
                
                // Weight label
                const lbl = document.createElement('div');
                lbl.className = 'absolute bg-white px-1 text-[10px] font-bold text-gray-500 rounded border border-gray-100 z-10';
                lbl.innerText = '1';
                lbl.style.left = (p1.x + p2.x)/2 + 'px'; lbl.style.top = (p1.y + p2.y)/2 + 'px';
                lbl.style.transform = 'translate(-50%, -50%)';
                canvas.appendChild(lbl);
            });
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

        window.onresize = () => { drawEdges(); drawNodes(); if(finalPath.length) highlightPath(finalPath); };
        setTimeout(() => { drawEdges(); drawNodes(); }, 150);

        function logMsg(msg) {
            const l = document.getElementById('log');
            l.innerHTML += `<div class="py-0.5 whitespace-nowrap"><span class="text-indigo-400 mr-2 text-[10px]">[${new Date().toLocaleTimeString('en-US',{hour12:false})}]</span> <span class="text-gray-700">${msg}</span></div>`;
            l.scrollTop = l.scrollHeight;
        }

        function select(i) {
            if (start === null) {
                start = i; document.getElementById('n'+i).classList.add('start'); document.getElementById('start-label').innerText = i;
                logMsg(`Start node configured: ${i}`);
            } else if (end === null && i !== start) {
                end = i; document.getElementById('n'+i).classList.add('end'); document.getElementById('end-label').innerText = i;
                logMsg(`End node configured: ${i}`);
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
                let el = document.getElementById('e_'+Math.min(p[i],p[i+1])+'_'+Math.max(p[i],p[i+1]));
                if(el) el.classList.add('path');
            }
        }

        function run() {
            if (start === null || end === null) return alert("Select Start & End node on canvas first!");
            let algo = document.getElementById('algo').value;
            ws.send(JSON.stringify({start, end, algo: algo==='dijkstra'?'astar':algo}));
            
            document.getElementById('robot-badge').innerText = 'Navigating';
            document.getElementById('robot-badge').className = 'text-[9px] font-bold tracking-widest text-indigo-700 bg-indigo-100 px-1.5 py-0.5 rounded uppercase';
            document.getElementById('progress-bar').style.width = '0%';
            logMsg(`Algorithm: ${algo.toUpperCase()} triggered`);
        }

        ws.onmessage = (msg) => {
            let data = JSON.parse(msg.data);
            if (data.path) {
                finalPath = data.path; highlightPath(finalPath);
                document.getElementById('path-result').innerHTML = finalPath.length ? '<div class="text-indigo-600 font-bold bg-indigo-50 px-3 py-1 rounded-full">' + finalPath.join(' <i class="fa-solid fa-arrow-right text-[10px] opacity-50 mx-0.5"></i> ') + '</div>' : 'No path';
                document.getElementById('cost-val').innerText = finalPath.length ? finalPath.length - 1 : 0;
                document.getElementById('nodes-val').innerText = finalPath.length;
                logMsg(`Path found: Cost=${finalPath.length-1}`);
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
</html>"""

import re
with open('/Users/thaongan/Downloads/CarLine/CarLine.ino', 'r') as f:
    text = f.read()

target_pattern = re.compile(r'(String html = R"rawliteral\().*?(\)rawliteral";)', re.DOTALL)
new_text = target_pattern.sub(lambda m: m.group(1) + "\n" + new_html + "\n" + m.group(2), text)

with open('/Users/thaongan/Downloads/CarLine/CarLine.ino', 'w') as f:
    f.write(new_text)

print("SUCCESS")
