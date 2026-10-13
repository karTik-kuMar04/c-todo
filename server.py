#!/usr/bin/env python3
import http.server
import socketserver
import json
import os
import urllib.parse
from pathlib import Path

PORT = 8080
BASE_DIR = Path(__file__).resolve().parent

# Check if tasks.json exists in root, otherwise ~/.todo/tasks.json
STORAGE_ENV = os.environ.get("TODO_TASKS_FILE")
if STORAGE_ENV:
    DATA_FILE = Path(STORAGE_ENV)
else:
    # Use current working directory tasks.json if present, else ~/.todo/tasks.json
    local_file = BASE_DIR / "tasks.json"
    if local_file.exists():
        DATA_FILE = local_file
    else:
        DATA_FILE = Path.home() / ".todo" / "tasks.json"

DATA_FILE.parent.mkdir(parents=True, exist_ok=True)
if not DATA_FILE.exists():
    with open(DATA_FILE, "w", encoding="utf-8") as f:
        json.dump([], f)

def read_tasks():
    try:
        with open(DATA_FILE, "r", encoding="utf-8") as f:
            return json.load(f)
    except Exception:
        return []

def write_tasks(tasks):
    DATA_FILE.parent.mkdir(parents=True, exist_ok=True)
    with open(DATA_FILE, "w", encoding="utf-8") as f:
        json.dump(tasks, f, indent=4)

class TodoApiHandler(http.server.SimpleHTTPRequestHandler):
    def __init__(self, *args, **kwargs):
        super().__init__(*args, directory=str(BASE_DIR / "web"), **kwargs)

    def do_GET(self):
        parsed = urllib.parse.urlparse(self.path)
        if parsed.path == "/api/tasks":
            tasks = read_tasks()
            self.send_response(200)
            self.send_header("Content-Type", "application/json")
            self.send_header("Access-Control-Allow-Origin", "*")
            self.end_headers()
            self.wfile.write(json.dumps(tasks).encode("utf-8"))
            return

        # Serve static web files
        return super().do_GET()

    def do_POST(self):
        parsed = urllib.parse.urlparse(self.path)
        if parsed.path == "/api/tasks":
            length = int(self.headers.get("Content-Length", 0))
            body = json.loads(self.rfile.read(length).decode("utf-8"))

            tasks = read_tasks()
            next_id = max([t.get("id", 0) for t in tasks], default=0) + 1
            new_task = {
                "id": next_id,
                "title": body.get("title", "Untitled"),
                "priority": body.get("priority", "med"),
                "done": False
            }
            tasks.append(new_task)
            write_tasks(tasks)

            self.send_response(201)
            self.send_header("Content-Type", "application/json")
            self.end_headers()
            self.wfile.write(json.dumps(new_task).encode("utf-8"))
            return

        if parsed.path.startswith("/api/tasks/") and parsed.path.endswith("/toggle"):
            parts = parsed.path.strip("/").split("/")
            task_id = int(parts[2])
            tasks = read_tasks()
            for t in tasks:
                if t.get("id") == task_id:
                    t["done"] = not t.get("done", False)
                    break
            write_tasks(tasks)

            self.send_response(200)
            self.send_header("Content-Type", "application/json")
            self.end_headers()
            self.wfile.write(json.dumps({"success": True}).encode("utf-8"))
            return

        self.send_error(404)

    def do_DELETE(self):
        parsed = urllib.parse.urlparse(self.path)
        if parsed.path.startswith("/api/tasks/"):
            task_id = int(parsed.path.strip("/").split("/")[2])
            tasks = read_tasks()
            tasks = [t for t in tasks if t.get("id") != task_id]
            write_tasks(tasks)

            self.send_response(200)
            self.send_header("Content-Type", "application/json")
            self.end_headers()
            self.wfile.write(json.dumps({"success": True}).encode("utf-8"))
            return

        self.send_error(404)

    def do_OPTIONS(self):
        self.send_response(200)
        self.send_header("Access-Control-Allow-Origin", "*")
        self.send_header("Access-Control-Allow-Methods", "GET, POST, DELETE, OPTIONS")
        self.send_header("Access-Control-Allow-Headers", "Content-Type")
        self.end_headers()

if __name__ == "__main__":
    socketserver.TCPServer.allow_reuse_address = True
    with socketserver.TCPServer(("", PORT), TodoApiHandler) as httpd:
        print(f"=====================================================")
        print(f" TaskFlow Visual App running at: http://localhost:{PORT}")
        print(f" Syncing with persistence file: {DATA_FILE}")
        print(f"=====================================================")
        try:
            httpd.serve_forever()
        except KeyboardInterrupt:
            print("\nShutting down server.")
