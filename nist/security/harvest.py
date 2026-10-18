#!/usr/bin/env python3
"""Fetch registered public research sources and append an auditable fetch log."""
import argparse
import datetime
import fcntl
import hashlib
from html.parser import HTMLParser
import json
import os
from pathlib import Path
import shutil
import subprocess
import sys
import tempfile
import requests

ROOT = Path(__file__).resolve().parents[1]
REGISTRY = ROOT / "security/sources.json"
BUNDLED_LOG = ROOT / "security/harvests.jsonl"
LOG = ROOT / ".cache/security/harvests.jsonl"
OBJECTS = ROOT / ".cache/security/objects"
MAX_BYTES = 32 * 1024 * 1024


class Text(HTMLParser):
    def __init__(self):
        super().__init__(convert_charrefs=True)
        self.ignored = 0
        self.parts = []

    def handle_starttag(self, tag, attrs):
        if tag in {"script", "style", "noscript"}:
            self.ignored += 1
        elif tag in {"p", "div", "br", "tr", "h1", "h2", "h3", "li"}:
            self.parts.append("\n")

    def handle_endtag(self, tag):
        if tag in {"script", "style", "noscript"} and self.ignored:
            self.ignored -= 1
        elif tag in {"p", "div", "tr", "h1", "h2", "h3", "li"}:
            self.parts.append("\n")

    def handle_data(self, data):
        if not self.ignored:
            self.parts.append(data)


def sha(data):
    return hashlib.sha256(data).hexdigest()


def store(data):
    digest = sha(data)
    target = OBJECTS / digest
    reused = target.exists()
    if not reused:
        OBJECTS.mkdir(parents=True, exist_ok=True)
        with tempfile.NamedTemporaryFile(dir=OBJECTS, delete=False) as temporary:
            temporary.write(data)
            name = temporary.name
        os.replace(name, target)
    elif sha(target.read_bytes()) != digest:
        raise ValueError(f"existing cache object has incorrect bytes: {target}")
    return digest, target.relative_to(ROOT).as_posix(), reused


def extracted_text(data, content_type):
    if data.startswith((b'\x1f\x8b', b'PK\x03\x04')):
        return None, "binary archive; no text extraction"
    if data.startswith(b"%PDF-"):
        executable = shutil.which("pdftotext")
        if not executable:
            return None, "pdftotext unavailable"
        process = subprocess.run([executable, "-layout", "-", "-"], input=data,
                                 capture_output=True, timeout=30)
        if process.returncode:
            return None, "pdftotext failed"
        return process.stdout.decode("utf-8", errors="replace"), None
    text = data.decode("utf-8", errors="replace")
    if "html" in content_type or "<html" in text[:500].lower():
        parser = Text()
        parser.feed(text)
        text = "".join(parser.parts)
        text = "\n".join(" ".join(line.split()) for line in text.splitlines() if line.strip())
    return text, None


def append(record):
    LOG.parent.mkdir(parents=True, exist_ok=True)
    with LOG.open("a", encoding="utf-8") as stream:
        fcntl.flock(stream, fcntl.LOCK_EX)
        stream.write(json.dumps(record, sort_keys=True, ensure_ascii=True) + "\n")
        stream.flush()
        os.fsync(stream.fileno())


def harvest(source):
    record = {"source_id": source["id"], "requested_url": source["url"],
              "retrieved_at": datetime.datetime.now(datetime.timezone.utc).isoformat(),
              "source_config_sha256": sha(json.dumps(source, sort_keys=True).encode()),
              "network_attempt": True, "from_cache": False, "status": "error"}
    if source.get("expected_sha256"):
        record["expected_sha256"] = source["expected_sha256"]
    try:
        with requests.get(source["url"], timeout=(15, 30), stream=True,
                          headers={"User-Agent": "ngcc-nist-research-harvester/1.0"}) as response:
            record.update(final_url=response.url, http_status=response.status_code,
                          redirects=[{"url": item.url, "status": item.status_code} for item in response.history],
                          content_type=response.headers.get("Content-Type", ""))
            chunks, size = [], 0
            for chunk in response.iter_content(65536):
                size += len(chunk)
                if size > MAX_BYTES:
                    raise ValueError("response exceeds 32 MiB limit")
                chunks.append(chunk)
            data = b"".join(chunks)
            digest, path, reused = store(data)
            record.update(bytes=len(data), sha256=digest, cache_path=path, object_reused=reused)
            text, error = extracted_text(data, record["content_type"])
            if text is not None:
                text_digest, text_path, _ = store(text.encode("utf-8"))
                record.update(text_sha256=text_digest, text_cache_path=text_path)
            if error:
                record["text_error"] = error
            if not 200 <= response.status_code < 300:
                record.update(status="http_error", error=f"HTTP {response.status_code}")
            elif source.get("expected_sha256") and digest != source["expected_sha256"]:
                record.update(status="hash_mismatch", error="response differs from pinned source digest")
            elif any(token.casefold() not in (text or "").casefold() for token in source.get("required_text", [])):
                record.update(status="content_check_failed", error="required source text absent; possible login/challenge/interstitial")
            else:
                record["status"] = "ok"
    except (requests.RequestException, OSError, ValueError, subprocess.SubprocessError) as error:
        record["error"] = f"{type(error).__name__}: {error}"
    append(record)
    print(f"{source['id']}: {record['status']} HTTP={record.get('http_status', '-')} "
          f"sha256={record.get('sha256', '-')}" + (f" ({record['error']})" if "error" in record else ""), flush=True)
    return record["status"] == "ok"


def verify(sources):
    records = []
    for log in (BUNDLED_LOG, LOG):
        if log.exists():
            records.extend(json.loads(line) for line in log.read_text().splitlines() if line)
    failed = 0
    for source in sources:
        matches = [record for record in records if record["source_id"] == source["id"] and record["status"] == "ok"]
        if not matches:
            print(f"{source['id']}: NO SUCCESSFUL HARVEST")
            failed += 1
            continue
        record = matches[-1]
        valid = not source.get("expected_sha256") or record["sha256"] == source["expected_sha256"]
        for digest_key, path_key in [("sha256", "cache_path"), ("text_sha256", "text_cache_path")]:
            if path_key not in record:
                continue
            path = ROOT / record[path_key]
            valid = valid and path.is_file() and sha(path.read_bytes()) == record[digest_key]
        print(f"{source['id']}: {'CACHE VERIFIED' if valid else 'CACHE MISSING/CORRUPT'} ({record['retrieved_at']})")
        failed += not valid
    return not failed


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("source_ids", nargs="*")
    parser.add_argument("--list", action="store_true", help="list registered URLs without network access")
    parser.add_argument("--verify-cache", action="store_true", help="verify latest successful harvested artifacts offline")
    args = parser.parse_args()
    sources = json.loads(REGISTRY.read_text())
    known = {source["id"] for source in sources}
    if len(known) != len(sources):
        parser.error("duplicate IDs in sources.json")
    if set(args.source_ids) - known:
        parser.error("unknown source IDs: " + ", ".join(sorted(set(args.source_ids) - known)))
    selected = [source for source in sources if not args.source_ids or source["id"] in args.source_ids]
    if args.list:
        for source in selected:
            print(source["id"] + "\t" + source["url"])
        return 0
    if args.verify_cache:
        return int(not verify(selected))
    successes = [harvest(source) for source in selected]
    return int(not all(successes))


if __name__ == "__main__":
    sys.exit(main())
