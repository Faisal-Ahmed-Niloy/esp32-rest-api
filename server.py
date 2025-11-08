# server.py
from fastapi import FastAPI, Request, Form
from fastapi.responses import HTMLResponse, JSONResponse
from fastapi.middleware.cors import CORSMiddleware
import uvicorn
from datetime import datetime
import threading

app = FastAPI()

# Allow CORS for LAN testing
app.add_middleware(
    CORSMiddleware,
    allow_origins=["*"],
    allow_credentials=True,
    allow_methods=["*"],
    allow_headers=["*"],
)

# store-local
targets = {}      # { "A123": 10, "B456": 5, ... }
logs = []         # list of received JSON logs (from ESP32)
notifications = []  # maintenance messages

@app.get("/", response_class=HTMLResponse)
def ui():
    # minimal UI - small JS to set/get target for selected user
    return """
<!doctype html>
<html>
<head>
  <meta charset="utf-8"/>
  <title>GMS</title>
  <style>
    body { font-family: Arial, sans-serif; margin: 20px; }
    label, select, input { font-size: 16px; margin: 6px; }
    .btn { padding: 8px 12px; font-size: 16px; margin:6px; }
    .card { border:1px solid #ddd; padding:12px; margin-top:12px; border-radius:6px; }
    pre { background:#f6f6f6; padding:8px; overflow:auto; }
  </style>
</head>
<body>
  <h2>Garments Management System (GMS)</h2>

  <div>
    <label>User:</label>
    <select id="user">
      <option value="A123">A123</option>
      <option value="B456">B456</option>
      <option value="C789">C789</option>
      <option value="etc">etc</option>
    </select>
    <label>Today's Target:</label>
    <input id="target" type="number" value="1" min="0" />
    <button class="btn" onclick="setTarget()">Send Target → ESP32</button>
    <button class="btn" onclick="getTarget()">Get Target</button>
  </div>

  <div class="card">
    <h4>Latest target for user:</h4>
    <div id="latest">—</div>
  </div>

  <div class="card">
    <h4>Recent logs (from ESP32)</h4>
    <div id="logs">No logs yet</div>
    <button class="btn" onclick="refreshLogs()">Refresh</button>
  </div>

  <div class="card">
    <h4>Notifications (maintenance)</h4>
    <div id="notes">No notifications</div>
    <button class="btn" onclick="refreshNotifications()">Refresh</button>
  </div>

<script>
async function setTarget(){
  const user = document.getElementById('user').value;
  const target = parseInt(document.getElementById('target').value);
  const res = await fetch('/set_target', {
    method:'POST',
    headers: {'Content-Type':'application/json'},
    body: JSON.stringify({user:user, target: target})
  });
  const j = await res.json();
  document.getElementById('latest').innerText = JSON.stringify(j);
}

async function getTarget(){
  const user = document.getElementById('user').value;
  const res = await fetch(`/get_target?user=${user}`);
  const j = await res.json();
  document.getElementById('latest').innerText = JSON.stringify(j);
}

async function refreshLogs(){
  const r = await fetch('/logs');
  const j = await r.json();
  if (j.length === 0) document.getElementById('logs').innerText = 'No logs yet';
  else {
    document.getElementById('logs').innerHTML = '<pre>' + JSON.stringify(j.slice(-20).reverse(), null, 2) + '</pre>';
  }
}

async function refreshNotifications(){
  const r = await fetch('/notifications');
  const j = await r.json();
  if (j.length === 0) document.getElementById('notes').innerText = 'No notifications';
  else document.getElementById('notes').innerHTML = '<pre>' + JSON.stringify(j.slice(-20).reverse(), null, 2) + '</pre>';
}

// refresh on load
refreshLogs();
refreshNotifications();
</script>

</body>
</html>
"""

@app.post("/set_target")
async def set_target(payload: Request):
    data = await payload.json()
    user = data.get("user")
    target = data.get("target")
    if not user or target is None:
        return JSONResponse({"error":"user and target required"}, status_code=400)
    targets[user] = int(target)
    print(f"[{datetime.now()}] Set target: {user} -> {target}")
    return {"ok": True, "user": user, "target": targets[user]}

@app.get("/get_target")
async def get_target(user: str):
    # returns {user, target} even if not set (target 0)
    tgt = targets.get(user, 0)
    return {"user": user, "target": tgt}

@app.post("/data")
async def receive_data(request: Request):
    # Accept logs from ESP32 (button presses)
    payload = await request.json()
    payload['_received_at'] = datetime.now().isoformat()
    logs.append(payload)
    print("Received from ESP32:", payload)
    return {"status": "ok", "received": payload}

@app.post("/notify")
async def receive_notify(request: Request):
    payload = await request.json()
    payload['_received_at'] = datetime.now().isoformat()
    notifications.append(payload)
    print("Maintenance notification:", payload)
    return {"status":"ok", "received": payload}

@app.get("/logs")
def get_logs():
    # return most recent 100 logs
    return logs[-100:]

@app.get("/notifications")
def get_notifications():
    return notifications[-100:]

if __name__ == "__main__":
    # run uvicorn
    uvicorn.run(app, host="192.168.30.31", port=8000)
