from fastapi import FastAPI, Request, Form
from fastapi.responses import HTMLResponse
from fastapi.middleware.cors import CORSMiddleware
from datetime import datetime
import uvicorn

app = FastAPI()

# --- Allow CORS for ESP32 ---
app.add_middleware(
    CORSMiddleware,
    allow_origins=["*"],
    allow_methods=["*"],
    allow_headers=["*"]
)

# --- In-memory storage for targets ---
users_targets = {
    "A123": 10,
    "B456": 5,
    "C789": 8
}

# --- REST API Endpoints ---

@app.get("/get_target")
def get_target(user: str):
    """ESP32 fetches the target for a user"""
    target = users_targets.get(user, 0)
    return {"user": user, "target": target}

@app.post("/data")
async def receive_data(request: Request):
    """ESP32 sends work done or maintenance info"""
    data = await request.json()
    print(f"[{datetime.now()}] Received from ESP32: {data}")
    return {"status": "ok", "received": data}

# --- Web UI ---

@app.get("/", response_class=HTMLResponse)
def root():
    html_content = """
    <html>
    <head>
        <title>ESP32 Target Dashboard</title>
        <style>
            body { font-family: Arial, sans-serif; background-color: #f0f0f0; padding: 20px; }
            h2 { color: #333; }
            table { border-collapse: collapse; width: 50%; margin-bottom: 20px; }
            th, td { border: 1px solid #999; padding: 8px; text-align: center; }
            th { background-color: #555; color: white; }
            td { background-color: #fff; }
            input[type=number] { width: 60px; padding: 5px; }
            select { padding: 5px; }
            input[type=submit] { padding: 5px 10px; }
        </style>
    </head>
    <body>
        <h2>ESP32 Target Dashboard</h2>
        <h3>Current Targets:</h3>
        <table>
            <tr><th>User</th><th>Target</th></tr>
    """
    for user, target in users_targets.items():
        html_content += f"<tr><td>{user}</td><td>{target}</td></tr>"

    html_content += """
        </table>
        <h3>Update Target:</h3>
        <form action="/set_target" method="post">
            Select User:
            <select name="user">
    """
    for user in users_targets.keys():
        html_content += f'<option value="{user}">{user}</option>'

    html_content += """
            </select>
            Target: <input type="number" name="target" value="10">
            <input type="submit" value="Set Target">
        </form>
    </body>
    </html>
    """
    return HTMLResponse(content=html_content)

@app.post("/set_target")
def set_target(user: str = Form(...), target: int = Form(...)):
    users_targets[user] = target
    return f"""
    <html>
    <body>
        <h3>Target for {user} set to {target}</h3>
        <a href="/">Go back</a>
    </body>
    </html>
    """

# --- Run server ---
if __name__ == "__main__":
    uvicorn.run(app, host="192.168.30.31", port=8000)
