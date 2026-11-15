#!/bin/bash
# Builds NFSHPR_SingleRace.asi with clang + the system MinGW-w64 headers and import libraries
# (Ubuntu package g++-mingw-w64-x86-64), without the MinGW CRT startup code (C library = msvcrt.dll).
# Usage: build_mingw.sh <imgui dir> <output dir>
# Dear ImGui 1.91.5 (MIT): _mod_sources\NFSHPR-EventRandomizer-main\third_party\imgui.
set -e
cd "$(dirname "$0")"
IMGUI=$(cd "$1" && pwd); B=$(mkdir -p "$2" && cd "$2" && pwd)
MW=/usr/x86_64-w64-mingw32
FLAGS="-I$IMGUI -I$IMGUI/backends -DIMGUI_DISABLE_WIN32_DEFAULT_IME_FUNCTIONS -DIMGUI_DISABLE_DEFAULT_SHELL_FUNCTIONS -DIMGUI_DISABLE_FILE_FUNCTIONS -DIMGUI_IMPL_WIN32_DISABLE_GAMEPAD -DIMGUI_DISABLE_DEBUG_TOOLS --target=x86_64-w64-windows-gnu -nostdinc -isystem $(clang -print-resource-dir)/include -isystem $MW/include -std=c++17 -O2 -fno-exceptions -fno-rtti -fno-asynchronous-unwind-tables -fno-stack-protector -ffunction-sections -fdata-sections -fno-threadsafe-statics -D__USE_MINGW_ANSI_STDIO=0 -DWIN32_LEAN_AND_MEAN -DNOMINMAX -D_WIN32_WINNT=0x0A00 -DNDEBUG"
OBJS=""
for s in NFSHPR_SingleRace.cpp SR_Overlay.cpp CrtShim.cpp; do o=$B/${s%.cpp}.o; clang++ $FLAGS -Wall -c $s -o $o; OBJS="$OBJS $o"; done
for s in $IMGUI/imgui.cpp $IMGUI/imgui_draw.cpp $IMGUI/imgui_tables.cpp $IMGUI/imgui_widgets.cpp $IMGUI/backends/imgui_impl_dx11.cpp $IMGUI/backends/imgui_impl_win32.cpp; do
  o=$B/imgui_$(basename ${s%.cpp}).o; [ -f $o ] || clang++ $FLAGS -c $s -o $o; OBJS="$OBJS $o"; done
# exact import libraries for the symbols the objects need (as the original build.sh), with the export
# names taken from the MinGW import libraries; no MinGW CRT code is linked in
python3 -I - "$B" "$MW/lib" $OBJS <<'PY'
import sys,subprocess,os,re
B,L=sys.argv[1],sys.argv[2]; objs=sys.argv[3:]
def nm(f): return subprocess.run(['llvm-nm',f],capture_output=True,text=True).stdout.splitlines()
und=set(); defd=set()
for o in objs:
    for l in nm(o):
        p=l.split()
        if len(p)>=2 and p[-2]=='U': und.add(p[-1])
        elif len(p)>=3: defd.add(p[-1])
need={(s[6:] if s.startswith('__imp_') else s) for s in und}-defd
dlls=[('kernel32.dll','kernel32'),('user32.dll','user32'),('gdi32.dll','gdi32'),('msvcrt.dll','msvcrt'),('dwmapi.dll','dwmapi'),('d3dcompiler_47.dll','d3dcompiler_47')]
names={}
for dll,lib in dlls:
    names[dll]={l.split()[-1][6:] for l in nm(f'{L}/lib{lib}.a') if l.split() and l.split()[-1].startswith('__imp_')}
# msvcrt.dll exports these math functions; the MinGW import library replaces them with its own code
names['msvcrt.dll']|={'pow','log','exp','sqrt','sin','cos','tan','atan','atan2','acos','asin','floor','ceil','fmod','log10','ldexp','frexp','modf','fabs'}
assign={}; missing=[]
for s in sorted(need):
    for dll,_ in dlls:
        if s in names[dll]: assign.setdefault(dll,[]).append(s); break
    else: missing.append(s)
if missing: print('UNRESOLVED:',missing); sys.exit(1)
os.makedirs(B+'/lib',exist_ok=True)
for f in os.listdir(B+'/lib'): os.remove(B+'/lib/'+f)
for dll,syms in assign.items():
    base=dll[:-4]
    open(f'{B}/lib/{base}.def','w').write(f'LIBRARY {dll}\nEXPORTS\n'+'\n'.join(syms)+'\n')
    subprocess.run(['llvm-dlltool','-m','i386:x86-64','-d',f'{B}/lib/{base}.def','-l',f'{B}/lib/lib{base}.a'],check=True)
    print(f'{dll}: {len(syms)} imports')
PY
LIBS=$(ls $B/lib/*.a)
ld.lld -m i386pep --shared --entry DllMain --subsystem windows --gc-sections -o $B/NFSHPR_SingleRace.asi $OBJS $LIBS
ls -l $B/NFSHPR_SingleRace.asi
