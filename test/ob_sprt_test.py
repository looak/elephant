#!/usr/bin/env python3
"""Checks test/ob_sprt.py against a stub of nyayo's /api/tests: the request's body and headers for a few command lines,
a 400's errors and warnings printed with a non-zero exit, --anyway, and --dry-run creating nothing.

    python test/ob_sprt_test.py

Nothing here reaches a real server or git remote: git and the branch checks are replaced.
"""
from __future__ import annotations

import contextlib
import http.server
import io
import json
import os
import sys
import threading
import unittest
from unittest import mock

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import ob_sprt  # noqa: E402

TOKEN = "s3cret"
PRESETS = {"engine": "Elephant", "default": "STC", "presets": {"STC": {"dev_tc": "8.0+0.08"}, "LTC": {"dev_tc": "40.0+0.4"}}}


class Stub(http.server.BaseHTTPRequestHandler):
    seen: list[dict] = []
    reply: tuple[int, dict] = (201, {"id": 7, "match": "m", "url": "http://nyayo/tests/7", "warnings": []})

    def log_message(self, *args):
        pass

    def send(self, status: int, payload: dict) -> None:
        raw = json.dumps(payload).encode()
        self.send_response(status)
        self.send_header("Content-Type", "application/json")
        self.send_header("Content-Length", str(len(raw)))
        self.end_headers()
        self.wfile.write(raw)

    def do_GET(self):
        Stub.seen.append({"method": "GET", "path": self.path, "auth": self.headers.get("Authorization")})
        self.send(200, PRESETS)

    def do_POST(self):
        length = int(self.headers.get("Content-Length", 0))
        Stub.seen.append({"method": "POST", "path": self.path, "auth": self.headers.get("Authorization"),
                          "type": self.headers.get("Content-Type"), "body": json.loads(self.rfile.read(length))})
        self.send(*Stub.reply)


class ObSprt(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.server = http.server.ThreadingHTTPServer(("127.0.0.1", 0), Stub)
        threading.Thread(target=cls.server.serve_forever, daemon=True).start()
        cls.url = f"http://127.0.0.1:{cls.server.server_port}"

    @classmethod
    def tearDownClass(cls):
        cls.server.shutdown()

    def setUp(self):
        Stub.seen = []
        Stub.reply = (201, {"id": 7, "match": "m", "url": "http://nyayo/tests/7", "warnings": []})
        patches = [
            mock.patch.object(ob_sprt, "git", return_value=""),
            mock.patch.object(ob_sprt, "resolve", return_value=("a" * 40, "123456")),
            mock.patch.dict(os.environ, {"NYAYO_URL": self.url, "NYAYO_TOKEN": TOKEN}),
        ]
        for patch in patches:
            patch.start()
            self.addCleanup(patch.stop)

    def run_script(self, *argv: str) -> tuple[int, str]:
        out = io.StringIO()
        with contextlib.redirect_stdout(out):
            try:
                code = ob_sprt.main(list(argv))
            except SystemExit as exit:
                code = exit.code if isinstance(exit.code, int) else (print(exit.code) or 1)
        return code, out.getvalue()

    def test_default_is_stc_with_nothing_overridden(self):
        code, out = self.run_script("dev", "base")
        self.assertEqual(code, 0)
        self.assertIn("http://nyayo/tests/7", out)
        (request,) = Stub.seen
        self.assertEqual(request["method"], "POST")
        self.assertEqual(request["path"], "/api/tests")
        self.assertEqual(request["auth"], f"Bearer {TOKEN}")
        self.assertEqual(request["type"], "application/json")
        self.assertEqual(request["body"], {"engine": "Elephant", "preset": "STC", "dev_branch": "dev", "base_branch": "base"})

    def test_ltc_nonreg_and_overrides_use_the_forms_field_names(self):
        code, _ = self.run_script("dev", "base", "--ltc", "--nonreg", "--tc", "8+0.08", "--options", "Threads=1 Hash=16",
                                  "--book", "Endgames.epd", "--win-adj", "None", "--draw-adj", "None", "--priority", "-1")
        self.assertEqual(code, 0)
        self.assertEqual(Stub.seen[0]["body"], {
            "engine": "Elephant", "preset": "LTC", "dev_branch": "dev", "base_branch": "base", "bounds": "[-3.00, 1.00]",
            "dev_tc": "8+0.08", "base_tc": "8+0.08", "dev_options": "Threads=1 Hash=16", "base_options": "Threads=1 Hash=16",
            "book": "Endgames.epd", "win_adj": "None", "draw_adj": "None", "priority": "-1"})

    def test_bounds_beat_nonreg_and_are_spelled_as_the_form_has_them(self):
        self.run_script("dev", "base", "--nonreg", "--bounds", "-1,4")
        self.assertEqual(Stub.seen[0]["body"]["bounds"], "[-1.00, 4.00]")

    def test_a_400_prints_errors_and_warnings_and_fails(self):
        Stub.reply = (400, {"errors": ["dev: no such branch"], "warnings": ["dev is behind base"]})
        code, out = self.run_script("dev", "base")
        self.assertNotEqual(code, 0)
        self.assertIn("dev: no such branch", out)
        self.assertIn("dev is behind base", out)
        self.assertNotIn("queued", out)
        self.assertNotIn("anyway", Stub.seen[0]["body"])

    def test_anyway_resends_with_anyway(self):
        code, _ = self.run_script("dev", "base", "--anyway")
        self.assertEqual(code, 0)
        self.assertIs(Stub.seen[0]["body"]["anyway"], True)

    def test_a_401_fails(self):
        Stub.reply = (401, {"errors": ["bad token"]})
        code, out = self.run_script("dev", "base")
        self.assertNotEqual(code, 0)
        self.assertIn("bad token", out)

    def test_dry_run_gets_the_presets_and_creates_nothing(self):
        code, out = self.run_script("dev", "base", "--ltc", "--dry-run")
        self.assertEqual(code, 0)
        self.assertEqual([r["method"] for r in Stub.seen], ["GET"])
        self.assertEqual(Stub.seen[0]["path"], "/api/tests/presets?engine=Elephant")
        self.assertEqual(Stub.seen[0]["auth"], f"Bearer {TOKEN}")
        self.assertIn('"preset": "LTC"', out)

    def test_no_token_stops_before_sending(self):
        with mock.patch.dict(os.environ, {}, clear=False):
            os.environ.pop("NYAYO_TOKEN")
            code, out = self.run_script("dev", "base")
        self.assertNotEqual(code, 0)
        self.assertIn("api_token create", out)
        self.assertEqual(Stub.seen, [])


if __name__ == "__main__":
    unittest.main()
