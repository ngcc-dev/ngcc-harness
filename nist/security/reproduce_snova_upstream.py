#!/usr/bin/env python3
"""Pinned upstream odd-prime SNOVA artifact: solver regression, small sample, full recipe.

verify (default) builds the author solver and runs deterministic regression.
sample additionally builds the pinned honest signer and verifies retained pairs.
recipe prints commands for the full attack; it never starts a large collection.
Downloads and generated binaries/pairs remain in ignored .cache/security.
"""
import argparse,hashlib,json,os,pathlib,re,shlex,shutil,subprocess,sys,tarfile,urllib.request
ROOT=pathlib.Path(__file__).resolve().parents[1]
PIN=json.loads((ROOT/'security/artifacts/SNOVA-LPN.json').read_text())
def sha(path):
    with path.open('rb') as stream:return hashlib.file_digest(stream,'sha256').hexdigest()
def fetch(entry,name,cache):
    archive=cache/(name+'-'+entry['commit']+'.tar.gz')
    if not archive.exists():
        temporary=archive.with_suffix('.part')
        request=urllib.request.Request(entry['archive_url'],headers={'User-Agent':'ngcc-nist-pinned-reproducer'})
        with urllib.request.urlopen(request,timeout=60) as src,temporary.open('wb') as dst:
            import shutil
            shutil.copyfileobj(src,dst)
        temporary.replace(archive)
    if archive.stat().st_size!=entry['archive_bytes'] or sha(archive)!=entry['archive_sha256']:
        raise SystemExit(f'archive identity mismatch: {archive}')
    with tarfile.open(archive) as bundle:
        top=pathlib.PurePosixPath(bundle.getmembers()[0].name).parts[0]
        if any(m.issym() or m.islnk() or m.isdev() for m in bundle.getmembers()):
            raise SystemExit('archive links/devices are not accepted')
        for member in bundle.getmembers():
            path=pathlib.PurePosixPath(member.name)
            if path.is_absolute() or '..' in path.parts:raise SystemExit('unsafe archive path')
        bundle.extractall(cache,filter='data')
    return cache/top

def run(command,log,*,timeout=900,env=None):
    print('+',shlex.join(map(str,command)),flush=True)
    result=subprocess.run(list(map(str,command)),cwd=ROOT,env=env,capture_output=True,text=True,timeout=timeout)
    log.write_text(result.stdout+result.stderr)
    if result.returncode:raise SystemExit(f'command failed ({result.returncode}); see {log}')
    return result.stdout

def main():
    if sys.version_info<(3,12):
        print('SKIP: optional upstream reproduction requires Python3.12+ for filtered archive extraction.',file=sys.stderr)
        return 77
    ap=argparse.ArgumentParser(description=__doc__)
    ap.add_argument('mode',choices=['verify','sample','recipe'],nargs='?',default='verify')
    ap.add_argument('--cache',type=pathlib.Path,default=ROOT/'.cache/security')
    ap.add_argument('--samples',type=int,default=64,help='small sample mode only, maximum10000')
    ap.add_argument('--set',choices=['A','B','C','D'],default='A')
    ap.add_argument('--workers',type=int,default=4)
    ap.add_argument('--recovery-jobs',type=int,default=4)
    ap.add_argument('--fft-threads',type=int,default=1)
    ap.add_argument('--cc',default=os.environ.get('CC','cc'))
    ap.add_argument('--cxx',default=os.environ.get('CXX','c++'))
    args=ap.parse_args()
    if not 1<=args.samples<=10000 or min(args.workers,args.recovery_jobs,args.fft_threads)<1:ap.error('invalid sample/job limits')
    cache=args.cache.resolve();cache.mkdir(parents=True,exist_ok=True)
    if args.mode!='recipe':
        required=[args.cxx]+([args.cc] if args.mode=='sample' else [])
        missing=[name for name in required if shutil.which(name) is None]
        if missing:
            print('SKIP: optional upstream SNOVA reproduction needs compiler(s): '+', '.join(missing),file=sys.stderr)
            return 77
        preflight=subprocess.run([args.cxx,'-x','c++','-std=c++17','-o',str(cache/'fftw-preflight'),'-','-lfftw3f','-lfftw3f_threads','-lm','-lpthread'],input='int main(){return 0;}\n',capture_output=True,text=True)
        if preflight.returncode:
            print('SKIP: optional upstream SNOVA reproduction needs working C++17 and single-precision FFTW development/thread libraries.',file=sys.stderr)
            (cache/'SNOVA-LPN-preflight.log').write_text(preflight.stdout+preflight.stderr)
            return 77
    source=fetch(PIN,'SNOVA-LPN',cache)
    for name,digest in PIN['files_sha256'].items():
        if sha(source/name)!=digest:raise SystemExit(f'upstream source mismatch: {name}')
    result={'artifact_commit':PIN['commit'],'mode':args.mode,'full_equivalent_key_recovery':False}
    if args.mode=='recipe':
        official=fetch(PIN['official_source'],'SNOVA-official',cache)
        p=PIN['full_experiment_defaults'][args.set];work=cache/('full-set-'+args.set)
        common=['--profile',p['profile'],'--workdir',str(work),'--retained',str(p['signatures']),'--workers',str(args.workers),'--filter','none']
        env=['env','GIT_CEILING_DIRECTORIES='+str(cache)]
        for stage in ['setup','stage1','stage2','stage3']:
            command=env+[sys.executable,str(source/'snova_round3_odd_recovery.py'),stage]+common
            if stage=='setup':command+=['--repo',str(official)]
            if stage=='stage3':command+=['--bkw-mode','lf2-lf2','--recovery-limit',str(p['signatures']),'--lf2-first-pair-limit',str(p['first_pair_limit']),'--lf2-pair-limit',str(p['terminal_pair_limit']),'--recovery-jobs',str(args.recovery_jobs),'--fft-threads',str(args.fft_threads)]
            print(shlex.join(command))
        print('Recipe only. Expect GB-scale retained data; Stage3 must independently validate the public equations and a fresh signature.')
        return 0
    solver=cache/'snova-bkw'
    run([args.cxx,'-std=c++17','-O2',source/'snova_round3_odd_bkw.cpp','-o',solver,'-lfftw3f','-lfftw3f_threads','-lm','-lpthread'],cache/'SNOVA-LPN-build.log')
    result['solver_self_test']=run([solver,'--self-test-lf2-lf2'],cache/'SNOVA-LPN-selftest.log').strip()
    result['synthetic_regression']=run([sys.executable,source/'check_generic_lf22_synthetic.py',solver],cache/'SNOVA-LPN-synthetic.log').strip()
    result['compiler']=subprocess.check_output([args.cxx,'--version'],text=True).splitlines()[0]
    try:
        result['fftw3f']=subprocess.check_output(['pkg-config','--modversion','fftw3f'],text=True,stderr=subprocess.DEVNULL).strip()
    except (OSError,subprocess.CalledProcessError):
        result['fftw3f']='link preflight passed; pkg-config version unavailable'
    if args.mode=='sample':
        official=fetch(PIN['official_source'],'SNOVA-official',cache)
        src=official/'src';work=cache/'small-q19-sample';work.mkdir(exist_ok=True)
        shim=work/'fixed_time.c';shim.write_text('#include <time.h>\ntime_t nist_artifact_time(time_t *out){time_t t=1700000000;if(out)*out=t;return t;}\n')
        flags=['-DSNOVA_OPT=0','-DSNOVA_v=27','-DSNOVA_o=4','-DSNOVA_q=19','-DSNOVA_l=4','-DSNOVA_r=6','-DSNOVA_m1=5','-DSNOVA_NAME=SNOVA_I_19_4x6','-DATTACK_CLASSES=5','-DATTACK_FILTER_DIGITS=26','-DATTACK_SPLIT0=5','-DATTACK_SPLIT1=5','-DATTACK_SPLIT2=6','-DAESCTR','-Dtime=nist_artifact_time']
        collector_text=(source/'snova_round3_odd_collector.c').read_text()
        entropy_start=collector_text.index('    FILE *f = fopen("/dev/urandom", "rb");',collector_text.index('static void seed_official_drbg('))
        entropy_end=collector_text.index('    for (unsigned i = 0; i < 4; i++) entropy[i]',entropy_start)
        collector_text=collector_text[:entropy_start]+'    for (unsigned i = 0; i < 48; ++i) entropy[i] = (uint8_t)i;\n'+collector_text[entropy_end:]
        collector=work/'collector-deterministic.c';collector.write_text(collector_text)
        result['collector_adaptation']='Only seed_official_drbg entropy read replaced by bytes00..2f; domain mixing unchanged; time()=1700000000.'
        result['collector_adapted_sha256']=sha(collector)
        helper=work/'collector';files=[src/(f+'.c') for f in ['snova_ref','symmetric_ref','rng','sign','aes']]
        run([args.cc,'-std=c11','-O2','-I'+str(src),*flags,collector,*files,shim,'-o',helper],work/'build.log')
        result['honest_signer_self_test']=run([helper,'nist-selftest'],work/'selftest.log').strip()
        pk,sk,raw,lpn=[work/n for n in ['pk.bin','sk.bin','raw.bin','lpn.bin']]
        run([helper,'keygen',pk,sk],work/'keygen.log')
        result['generate']=run([helper,'generate-raw',pk,sk,raw,str(args.samples),'0','none','1'],work/'generate.log').strip()
        result['extract']=run([helper,'extract-lpn',pk,raw,lpn,'none','1',str(args.samples)],work/'extract.log').strip()
        result['sample_count']=args.samples
        result['deterministic_time']=1700000000
        result['witnesses']={p.name:{'bytes':p.stat().st_size,'sha256':sha(p)} for p in [pk,raw,lpn]}
        expected=PIN.get('local_execution',{})
        if expected.get('sample_count')==args.samples and expected.get('witnesses'):
            if result['witnesses']!=expected['witnesses']:
                raise SystemExit('deterministic witness hashes differ from the recorded64-pair baseline')
            result['recorded_witness_match']=True
        result['official_source_commit']=PIN['official_source']['commit']
        result['honest_source_files_sha256']={p.name:sha(p) for p in files}
        expected_sources=expected.get('honest_source_files_sha256')
        if expected_sources:
            if result['honest_source_files_sha256'] != expected_sources:
                raise SystemExit('honest signer source hashes differ from the recorded baseline')
            result['recorded_source_hashes_match']=True
    (cache/'SNOVA-LPN-local.json').write_text(json.dumps(result,indent=2)+'\n')
    print(json.dumps(result,indent=2))
    return 0
if __name__=='__main__':sys.exit(main())
