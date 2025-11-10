from fastapi import FastAPI, Request
from fastapi.responses import HTMLResponse, JSONResponse
from fastapi.middleware.cors import CORSMiddleware
from fastapi.staticfiles import StaticFiles
from fastapi.templating import Jinja2Templates
import uvicorn
from datetime import datetime

app = FastAPI()

# Allow CORS for LAN testing
app.add_middleware(
    CORSMiddleware,
    allow_origins=["*"],
    allow_credentials=True,
    allow_methods=["*"],
    allow_headers=["*"],
)

# Mount static directory
app.mount("/static", StaticFiles(directory="static"), name="static")

# Templates directory
templates = Jinja2Templates(directory="templates")

# store-local
targets = {}
logs = []
notifications = []

@app.get("/", response_class=HTMLResponse)
def ui(request: Request):
    return templates.TemplateResponse("index.html", {"request": request})

@app.post("/set_target")
async def set_target(payload: Request):
    data = await payload.json()
    user = data.get("user")
    target = data.get("target")
    if not user or target is None:
        return JSONResponse({"error": "user and target required"}, status_code=400)
    targets[user] = int(target)
    print(f"[{datetime.now()}] Set target: {user} -> {target}")
    return {"ok": True, "user": user, "target": targets[user]}

@app.get("/get_target")
async def get_target(user: str):
    tgt = targets.get(user, 0)
    return {"user": user, "target": tgt}

@app.post("/data")
async def receive_data(request: Request):
    payload = await request.json()
    payload["_received_at"] = datetime.now().isoformat()
    logs.append(payload)
    print("Received from ESP32:", payload)
    return {"status": "ok", "received": payload}

@app.post("/notify")
async def receive_notify(request: Request):
    payload = await request.json()
    payload["_received_at"] = datetime.now().isoformat()
    notifications.append(payload)
    print("Maintenance notification:", payload)
    return {"status": "ok", "received": payload}

@app.get("/logs")
def get_logs():
    return logs[-100:]

@app.get("/notifications")
def get_notifications():
    return notifications[-100:]

if __name__ == "__main__":
    uvicorn.run(app, host="10.100.53.234", port=8000)