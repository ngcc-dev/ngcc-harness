#!/usr/bin/env python3
"""Fetch NIST packages, retain source trees/specifications, record provenance."""
import argparse, concurrent.futures, hashlib, html.parser, json, pathlib, shutil, stat, urllib.parse, zipfile
import requests
ROOT = pathlib.Path(__file__).resolve().parents[1]
BASE = 'https://csrc.nist.gov'
PAGE = BASE + '/Projects/pqc-dig-sig/round-3-additional-signatures'
NAMES = ['FAEST','HAWK','MAYO','MQOM','QR-UOV','SDitH','SNOVA','SQIsign','UOV']
class Rows(html.parser.HTMLParser):
    def __init__(self): super().__init__(); self.rows=[]; self.row=None; self.link=None
    def handle_starttag(self,tag,attrs):
        if tag=='tr': self.row={'text':'','links':[]}
        if tag=='a' and self.row is not None: self.link=dict(attrs).get('href')
    def handle_data(self,s):
        if self.row is not None:
            self.row['text']+=s+' '
            if self.link: self.row['links'].append((s.strip(),urllib.parse.urljoin(BASE,self.link)))
    def handle_endtag(self,tag):
        if tag=='a': self.link=None
        if tag=='tr' and self.row is not None: self.rows.append(self.row); self.row=None

def page_rows(url):
    r=requests.get(url,timeout=90); r.raise_for_status(); p=Rows(); p.feed(r.text); return p.rows

def fetch(url,path,expected_sha256=None,expected_bytes=None):
    def matches_pin(candidate):
        if expected_bytes is not None and candidate.stat().st_size != expected_bytes:
            return False
        return expected_sha256 is None or digest(candidate) == expected_sha256

    if path.exists():
        if matches_pin(path): return
        path.unlink()
    path.parent.mkdir(parents=True,exist_ok=True)
    partial=path.with_suffix(path.suffix+'.part')
    partial.unlink(missing_ok=True)
    try:
        with requests.get(url,stream=True,timeout=(30,300)) as r:
            r.raise_for_status()
            with partial.open('wb') as f:
                for b in r.iter_content(1024*1024): f.write(b)
        if not matches_pin(partial):
            raise ValueError(f'downloaded file differs from pinned provenance: {path.name}')
        partial.replace(path)
    except BaseException:
        partial.unlink(missing_ok=True)
        path.unlink(missing_ok=True)
        raise

def digest(path):
    h=hashlib.sha256()
    with path.open('rb') as f:
        for b in iter(lambda:f.read(1024*1024),b''):h.update(b)
    return h.hexdigest()

def process(name,row,round_no,page,expected=None):
    links=row['links']; archive=next(u for t,u in links if t.lower() == 'zip file')
    spec=next(u for t,u in links if 'specification' in t.lower())
    target=ROOT/name; target.mkdir(exist_ok=True)
    zip_path=ROOT/'.cache/downloads'/f'{name}.zip'
    archive_pin=expected['archive_sha256'] if expected else None
    archive_bytes=expected['archive_bytes'] if expected else None
    spec_pin=expected['spec_sha256'] if expected else None
    fetch(archive,zip_path,archive_pin,archive_bytes)
    spec_path=target/f'{name}-spec.pdf'; fetch(spec,spec_path,spec_pin)
    archive_sha=digest(zip_path); spec_sha=digest(spec_path)
    if expected:
        if archive_sha!=expected['archive_sha256'] or zip_path.stat().st_size!=expected['archive_bytes']:
            raise ValueError(f'{name}: downloaded archive differs from pinned provenance')
        if spec_sha!=expected['spec_sha256']:
            raise ValueError(f'{name}: specification differs from pinned provenance')
    # Keep a complete ignored extraction for vector/provenance work. Reject unsafe paths.
    extracted=ROOT/'.cache/extracted'/name; extracted.mkdir(parents=True,exist_ok=True)
    with zipfile.ZipFile(zip_path) as z:
        for member in z.infolist():
            p=pathlib.PurePosixPath(member.filename)
            if p.is_absolute() or '..' in p.parts: raise ValueError(f'unsafe ZIP path: {p}')
            if stat.S_ISLNK(member.external_attr >> 16): raise ValueError(f'symlink in ZIP: {p}')
        z.extractall(extracted)
    # Only source/build/documentation files; PDFs are represented by the canonical spec.
    suffixes={'.c','.h','.cc','.cpp','.hpp','.hh','.s','.asm','.inc','.in','.cmake','.py','.sh','.pl','.sage','.rs','.toml','.mk','.mak','.md','.txt','.ipp','.tcc','.macros','.inl','.j2','.scala','.supp','.sha256'}
    count=0
    for source in extracted.rglob('*'):
        if not source.is_file(): continue
        rel=source.relative_to(extracted)
        if any(p in {'__MACOSX','.git','.github','build','CMakeFiles'} for p in rel.parts):continue
        low='/'.join(rel.parts).lower()
        if any(w in low for w in ['ip_statement','ip statement','patent','intellectual_property','intellectual property']):continue
        special = source.name.startswith(('README','LICENSE','COPYING','Makefile','GNUmakefile')) or source.name in {'makefile','CMakeLists.txt','NOTICE','AUTHORS','Doxyfile','KAT_download_link.txt'}
        if source.suffix.lower() not in suffixes and not special:continue
        if source.suffix.lower()=='.txt' and not special and (source.stat().st_size>1024*1024 or any(w in low for w in ['kat','test_vector','testvector'])):continue
        dest=target/'submission'/rel; dest.parent.mkdir(parents=True,exist_ok=True); shutil.copy2(source,dest); count+=1
    result={'scheme':name,'round':round_no,'status':'withdrawn; last NIST-hosted Round 2 package' if name=='HAWK' else 'Round 3 candidate','page':page,'archive_url':archive,'archive_bytes':zip_path.stat().st_size,'archive_sha256':archive_sha,'spec_url':spec,'spec_path':str(spec_path.relative_to(ROOT)),'spec_sha256':spec_sha,'source_files':count}
    print(f'{name}: {zip_path.stat().st_size} bytes, {count} retained files',flush=True)
    # The public fetch leaves checked-in provenance pins unchanged.
    return result

def main():
    parser=argparse.ArgumentParser(); parser.add_argument('schemes',nargs='*',choices=NAMES)
    parser.add_argument('--discover',action='store_true',help='discover current NIST links instead of replaying pinned provenance')
    a=parser.parse_args()
    pins={}
    pinfile=ROOT/'data/submissions.json'
    if pinfile.exists() and not a.discover:
        pins={p['scheme']:p for p in json.loads(pinfile.read_text())}
    rows=page_rows(PAGE) if not pins else []
    oldpage=BASE+'/Projects/pqc-dig-sig/round-2-additional-signatures'
    oldrows=page_rows(oldpage) if not pins else []
    tasks=[]
    with concurrent.futures.ThreadPoolExecutor(max_workers=4) as pool:
        for name in a.schemes or NAMES:
            if name in pins:
                p=pins[name]
                row={'links':[('Zip file',p['archive_url']),('Specifications',p['spec_url'])]}
                tasks.append(pool.submit(process,name,row,p['round'],p['page'],p))
                continue
            row=next(r for r in rows if r['text'].split() and r['text'].split()[0]==name)
            round_no=3; page=PAGE
            if not any('.zip' in u.lower() for _,u in row['links']):
                if name!='HAWK': raise ValueError(f'no Round 3 ZIP for {name}')
                row=next(r for r in oldrows if r['text'].split() and r['text'].split()[0]==name);round_no=2;page=oldpage
            tasks.append(pool.submit(process,name,row,round_no,page))
        results=[f.result() for f in concurrent.futures.as_completed(tasks)]
    call_path=ROOT/'doc/NIST-call-for-proposals.pdf'
    fetch(
        BASE+'/csrc/media/Projects/pqc-dig-sig/documents/call-for-proposals-dig-sig-sept-2022.pdf',
        call_path,
        'd66c2427fee94568791c1626c38c0c8dcd1825c4991482a0225599efc9b3ad5b',
    )
    # The public fetch leaves data/submissions.json unchanged.
if __name__=='__main__': main()
