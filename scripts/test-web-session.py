#!/usr/bin/env python3

import argparse
import os
import signal
import subprocess
import sys
import tempfile
import threading
import time
from http.server import BaseHTTPRequestHandler, ThreadingHTTPServer
from urllib.parse import parse_qs, urlparse

COOKIE_NAME = "apvlvsess"
USERNAME = "alf"
PASSWORD = "apvlv"
SESSION_VALUE = "session-of-" + USERNAME
COOKIE_MAX_AGE = 86400
WAIT_TIMEOUT = 90
COMMIT_TIMEOUT = 45

LOGIN_FORM = """<!doctype html>
<html><head><meta charset="utf-8"><title>apvlv login</title></head>
<body>
<h1>Login required</h1>
<form method="post" action="/login">
<p><label>user <input name="user" value=""></label></p>
<p><label>password <input name="password" type="password" value=""></label></p>
<p><button type="submit">login</button></p>
</form>
<p>try {user} / {password}</p>
</body></html>
""".format(user=USERNAME, password=PASSWORD)

WELCOME = """<!doctype html>
<html><head><meta charset="utf-8"><title>apvlv session</title></head>
<body>
<h1>LOGGED IN</h1>
<p>user: {user}</p>
<p><a href="/logout">logout</a></p>
</body></html>
""".format(user=USERNAME)


class Handler(BaseHTTPRequestHandler):
    protocol_version = "HTTP/1.1"

    def log_message(self, fmt, *args):
        pass

    def session_value(self):
        raw = self.headers.get("Cookie", "")
        for part in raw.split(";"):
            name, _, value = part.strip().partition("=")
            if name == COOKIE_NAME:
                return value
        return None

    def record(self, status, logged_in):
        self.server.records.append(
            {
                "run": self.server.run_label,
                "method": self.command,
                "path": self.path,
                "cookie": self.session_value(),
                "status": status,
                "logged_in": logged_in,
            }
        )

    def send_body(self, status, body, headers=()):
        raw = body.encode("utf-8")
        self.send_response(status)
        self.send_header("Content-Type", "text/html; charset=utf-8")
        self.send_header("Content-Length", str(len(raw)))
        self.send_header("Cache-Control", "no-store")
        for key, value in headers:
            self.send_header(key, value)
        self.end_headers()
        self.wfile.write(raw)

    def redirect(self, location, headers=()):
        self.send_response(302)
        self.send_header("Location", location)
        self.send_header("Content-Length", "0")
        for key, value in headers:
            self.send_header(key, value)
        self.end_headers()

    def grant(self, target):
        cookie = "{name}={value}; Path=/; Max-Age={age}".format(
            name=COOKIE_NAME, value=SESSION_VALUE, age=COOKIE_MAX_AGE
        )
        self.record(302, False)
        self.redirect(target, [("Set-Cookie", cookie)])

    def do_GET(self):
        url = urlparse(self.path)
        query = parse_qs(url.query)

        if url.path == "/":
            if self.session_value() == SESSION_VALUE:
                self.record(200, True)
                self.send_body(200, WELCOME)
            else:
                self.record(302, False)
                self.redirect("/login")
            return

        if url.path == "/login":
            if (
                query.get("user", [None])[0] == USERNAME
                and query.get("password", [None])[0] == PASSWORD
            ):
                self.grant("/")
            else:
                self.record(200, False)
                self.send_body(200, LOGIN_FORM)
            return

        if url.path == "/logout":
            self.record(302, False)
            self.redirect(
                "/", [("Set-Cookie", COOKIE_NAME + "=; Path=/; Max-Age=0")]
            )
            return

        self.record(404, False)
        self.send_body(404, "<html><body>not found</body></html>")

    def do_POST(self):
        length = int(self.headers.get("Content-Length", "0"))
        fields = parse_qs(self.rfile.read(length).decode("utf-8"))
        if (
            urlparse(self.path).path == "/login"
            and fields.get("user", [None])[0] == USERNAME
            and fields.get("password", [None])[0] == PASSWORD
        ):
            self.grant("/")
        else:
            self.record(401, False)
            self.send_body(401, LOGIN_FORM)


def start_server():
    server = ThreadingHTTPServer(("127.0.0.1", 0), Handler)
    server.records = []
    server.run_label = "manual"
    threading.Thread(target=server.serve_forever, daemon=True).start()
    return server


def base_url(server):
    return "http://127.0.0.1:{port}".format(port=server.server_address[1])


def wait_for(predicate, timeout=WAIT_TIMEOUT):
    deadline = time.monotonic() + timeout
    while time.monotonic() < deadline:
        found = predicate()
        if found:
            return found
        time.sleep(0.25)
    return None


def find_cookie_db(root):
    for dirpath, _, filenames in os.walk(root):
        if "Cookies" in filenames:
            return os.path.join(dirpath, "Cookies")
    return None


def cookie_on_disk(cache_dir):
    path = find_cookie_db(cache_dir)
    if not path:
        return None
    with open(path, "rb") as handle:
        if COOKIE_NAME.encode("ascii") in handle.read():
            return path
    return None


def launch(apvlv, url, env, log_path):
    log = open(log_path, "wb")
    return (
        subprocess.Popen(
            [apvlv, url],
            env=env,
            stdout=log,
            stderr=subprocess.STDOUT,
            start_new_session=True,
        ),
        log,
    )


def stop(process, log):
    try:
        os.killpg(os.getpgid(process.pid), signal.SIGTERM)
        process.wait(timeout=10)
    except (ProcessLookupError, subprocess.TimeoutExpired):
        try:
            os.killpg(os.getpgid(process.pid), signal.SIGKILL)
        except ProcessLookupError:
            pass
        process.wait(timeout=10)
    log.close()


def requests_of(server, run, path):
    url_path = lambda record: urlparse(record["path"]).path
    return [
        r for r in server.records if r["run"] == run and url_path(r) == path
    ]


def run_test(apvlv, platform):
    server = start_server()
    base = base_url(server)
    workdir = tempfile.mkdtemp(prefix="apvlv-web-session-")
    home = os.path.join(workdir, "home")
    cache = os.path.join(workdir, "cache")
    config = os.path.join(workdir, "config")
    for path in (home, cache, config):
        os.makedirs(path)

    env = dict(os.environ)
    env.update(
        {
            "HOME": home,
            "XDG_CACHE_HOME": cache,
            "XDG_CONFIG_DIR": config,
            "QT_QPA_PLATFORM": platform,
        }
    )

    print("test server:      " + base)
    print("apvlv binary:     " + apvlv)
    print("isolated HOME:    " + home)
    print("isolated cache:   " + cache)

    server.run_label = "login"
    login_url = "{base}/login?user={user}&password={password}".format(
        base=base, user=USERNAME, password=PASSWORD
    )
    print("\nrun 1: " + login_url)
    process, log = launch(apvlv, login_url, env, os.path.join(workdir, "run1.log"))
    try:
        landed = wait_for(
            lambda: [r for r in requests_of(server, "login", "/") if r["logged_in"]]
        )
        if not landed:
            print("FAIL: apvlv never reached the logged-in page in run 1")
            return 1
        print("       logged in, waiting for the cookie to hit the disk")
        path = wait_for(lambda: cookie_on_disk(cache), COMMIT_TIMEOUT)
        if path:
            print("       cookie store: " + path)
        else:
            print("       no cookie store was written under " + cache)
    finally:
        stop(process, log)

    server.run_label = "restart"
    print("\nrun 2: " + base + "/")
    process, log = launch(apvlv, base + "/", env, os.path.join(workdir, "run2.log"))
    try:
        again = wait_for(lambda: requests_of(server, "restart", "/"))
    finally:
        stop(process, log)

    print("\nrequests:")
    for record in server.records:
        print(
            "  [{run}] {method} {path} -> {status} cookie={cookie}".format(**record)
        )

    if not again:
        print("\nFAIL: apvlv did not request the page after restart")
        return 1
    if not again[0]["logged_in"]:
        print(
            "\nFAIL: the session was lost, apvlv sent cookie="
            + str(again[0]["cookie"])
        )
        return 1

    print("\nPASS: the session survived the restart")
    return 0


def run_serve():
    server = start_server()
    base = base_url(server)
    print("login page: " + base + "/")
    print("credentials: {user} / {password}".format(user=USERNAME, password=PASSWORD))
    print("open it in apvlv with:  apvlv " + base + "/")
    print("press Ctrl-C to stop")
    try:
        while True:
            time.sleep(1)
    except KeyboardInterrupt:
        print("")
    return 0


def main():
    parser = argparse.ArgumentParser(
        description="check that apvlv's embedded browser keeps a login session"
    )
    parser.add_argument("mode", choices=("test", "serve"))
    parser.add_argument(
        "--apvlv",
        default=os.path.join(
            os.path.dirname(os.path.dirname(os.path.abspath(__file__))),
            "build",
            "src",
            "apvlv",
        ),
    )
    parser.add_argument("--platform", default="offscreen")
    args = parser.parse_args()
    if args.mode == "serve":
        return run_serve()
    return run_test(args.apvlv, args.platform)


if __name__ == "__main__":
    sys.exit(main())
