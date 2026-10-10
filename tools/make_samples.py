#!/usr/bin/env python3
"""Sample media for OpenPlay, all made here and all ours (MIT): a tune and a Boing ball.

    python3 tools/make_samples.py OUT_DIR

Writes:
  Boing.mod    a 4-channel ProTracker module of the tune (plays through the mod datatype / the Nursery)
  Boing.mp3    the same tune rendered (MP3, 44.1 kHz)
  Boing.mp4    the Boing ball bouncing to the tune, 320 x 180, H.264 + AAC
  Boing.cdxl   the same film for an AGA Amiga's chipset: CDXL, 160 x 100, 32 colours, 12 fps, 8-bit mono 11025 Hz

Needs ffmpeg (with libx264, libmp3lame, aac) on the path, and numpy. The CDXL is checked by decoding it with
ffmpeg's own CDXL demuxer after it is written.
"""
import struct, subprocess, sys, math, os, tempfile
from pathlib import Path
import numpy as np

# ---- the tune: a bouncy chiptune in A minor, 125 BPM, 4 patterns ------------------------------------
# ProTracker periods (Amiga, finetune 0) for C-1 .. B-3
PERIODS = [856,808,762,720,678,640,604,570,538,508,480,453, 428,404,381,360,339,320,302,285,269,254,240,226,
           214,202,190,180,170,160,151,143,135,127,120,113]
NOTE_NAMES = ["C","C#","D","D#","E","F","F#","G","G#","A","A#","B"]
def note(name, octv):  # "A", 2 -> period
    return PERIODS[(octv - 1) * 12 + NOTE_NAMES.index(name)]

# samples: 1 square bass, 2 saw lead, 3 kick, 4 hat, 5 chord pad (pulse)
def square(n, period_len=64, duty=0.5, vol=1.0):
    t = np.arange(n); return (np.where((t % period_len) < period_len * duty, 1.0, -1.0) * vol)
def saw(n, period_len=64, vol=1.0):
    t = np.arange(n); return (((t % period_len) / period_len) * 2 - 1) * vol
def kick(n=2000):
    t = np.arange(n) / 8363.0
    f = 120 * np.exp(-t * 18) + 35
    return np.sin(2 * np.pi * np.cumsum(f) / 8363.0) * np.exp(-t * 9)
def hat(n=900):
    rng = np.random.default_rng(7)
    return rng.uniform(-1, 1, n) * np.exp(-np.arange(n) / 150.0)
def pad(n, period_len=64):
    t = np.arange(n); return (np.where((t % period_len) < period_len * 0.25, 1.0, -1.0) * 0.6 +
                             np.where(((t + period_len // 3) % period_len) < period_len * 0.5, 0.4, -0.4))

SAMPLES = [  # name, data (float -1..1), loop (start, len) in samples or None, volume 0..64
    ("square bass", square(64), (0, 64), 48),
    ("saw lead", saw(64), (0, 64), 40),
    ("kick", kick(), None, 64),
    ("hat", hat(), None, 36),
    ("pulse pad", pad(64), (0, 64), 30),
]

# a row: (note period or 0, sample 1.., effect, param)
def rows(spec):  # spec: list of 64 strings "A-2 s" / "---" / "A-2 s C20" (effect)
    out = []
    for s in spec:
        if s.startswith("---"): out.append((0, 0, 0, 0)); continue
        parts = s.split()
        nm, oc = parts[0][:-1].replace("-", ""), int(parts[0][-1])
        smp = int(parts[1])
        eff, par = 0, 0
        if len(parts) > 2:
            eff, par = int(parts[2][0], 16), int(parts[2][1:], 16)
        out.append((note(nm, oc), smp, eff, par))
    return out

def bass_line(root_seq):
    """root_seq: 4 roots (name, octave) for the 4 bars of a pattern; a bouncing octave bass."""
    r = []
    for (nm, oc) in root_seq:
        for beat in range(4):
            r += [f"{nm}-{oc} 1", "---", f"{nm}-{oc + 1} 1", "---"]
    return r

def drums():
    r = []
    for beat in range(16):
        r += ["C-2 3" if beat % 2 == 0 else "---", "C-3 4", "C-3 4" if beat % 2 else "---", "C-3 4"]
    return r

def lead(melody):  # melody: 64 entries (name, octave) or None
    return [f"{m[0]}-{m[1]} 2" if m else "---" for m in melody]

def padline(root_seq):
    r = []
    for (nm, oc) in root_seq:
        r += [f"{nm}-{oc} 5"] + ["---"] * 15
    return r

A = [("A", 2), ("F", 2), ("C", 2), ("G", 2)]
B = [("A", 2), ("F", 2), ("D", 2), ("E", 2)]
mel1 = [("A",3),None,("C",3),None,("E",3),None,("A",3),None, ("G",3),None,None,None,("E",3),None,None,None,
        ("F",3),None,("A",3),None,("C",3),None,("F",3),None, ("E",3),None,None,None,("C",3),None,None,None,
        ("C",3),None,("E",3),None,("G",3),None,("C",3),None, ("B",3),None,None,None,("G",3),None,None,None,
        ("G",3),None,("B",3),None,("D",3),None,("G",3),None, ("E",3),None,("D",3),None,("B",3),None,None,None]
mel2 = [("E",3),None,None,("E",3),None,None,("C",3),None, ("A",3),None,None,None,None,None,None,None,
        ("F",3),None,None,("F",3),None,None,("A",3),None, ("C",3),None,None,None,None,None,None,None,
        ("D",3),None,None,("D",3),None,None,("F",3),None, ("A",3),None,None,None,("G",3),None,("F",3),None,
        ("E",3),None,None,("E",3),None,None,("G",3),None, ("B",3),None,None,None,None,None,None,None]
PATTERNS = [
    [rows(bass_line(A)), rows(lead(mel1)), rows(drums()), rows(padline(A))],
    [rows(bass_line(B)), rows(lead(mel2)), rows(drums()), rows(padline(B))],
    [rows(bass_line(A)), rows(lead(mel1)), rows(drums()), rows(padline(A))],
    [rows(bass_line(B)), rows(lead([None] * 48 + mel2[48:])), rows(drums()), rows(padline(B))],
]
ORDER = [0, 1, 2, 3]
BPM, SPEED = 125, 6                      # ProTracker's defaults: 6 ticks a row at 125 BPM = 0.12 s a row

# ---- ProTracker writer -------------------------------------------------------------------------------
def write_mod(path):
    out = bytearray()
    out += b"AmigaChrome Boing".ljust(20, b"\0")
    for i in range(31):
        if i < len(SAMPLES):
            name, data, loop, vol = SAMPLES[i]
            pcm = np.clip(data * 127, -128, 127).astype(np.int8).tobytes()
            if len(pcm) % 2: pcm += b"\0"
            n = len(pcm) // 2
            ls, ll = (loop[0] // 2, loop[1] // 2) if loop else (0, 1)
            out += name.encode().ljust(22, b"\0") + struct.pack(">HBBHH", n, 0, vol, ls, ll)
        else:
            out += b"\0" * 22 + struct.pack(">HBBHH", 0, 0, 0, 0, 1)
    out += struct.pack("BB", len(ORDER), 127)
    out += bytes(ORDER).ljust(128, b"\0")
    out += b"M.K."
    for pat in PATTERNS:
        for row in range(64):
            for ch in range(4):
                period, smp, eff, par = pat[ch][row]
                out += bytes([(smp & 0xF0) | (period >> 8), period & 0xFF, ((smp & 0x0F) << 4) | eff, par])
    for i in range(len(SAMPLES)):
        name, data, loop, vol = SAMPLES[i]
        pcm = np.clip(data * 127, -128, 127).astype(np.int8).tobytes()
        if len(pcm) % 2: pcm += b"\0"
        out += pcm
    Path(path).write_bytes(bytes(out))
    return len(out)

# ---- a simple renderer of the same module (Amiga period -> rate, Paula-ish, 4 channels) -------------
PAL_CLOCK = 3546895.0
def render(rate=44100):
    row_s = 2.5 / BPM * SPEED               # seconds a row
    total_rows = len(ORDER) * 64
    n = int(row_s * total_rows * rate) + rate
    mix = np.zeros((n, 2), dtype=np.float64)
    chans = [dict(period=0, smp=0, pos=0.0, on=False) for _ in range(4)]
    pan = [0.25, 0.75, 0.75, 0.25]          # L R R L, as the Amiga's
    for oi, pi in enumerate(ORDER):
        pat = PATTERNS[pi]
        for row in range(64):
            t0 = int((oi * 64 + row) * row_s * rate)
            t1 = int((oi * 64 + row + 1) * row_s * rate)
            for ch in range(4):
                period, smp, eff, par = pat[ch][row]
                c = chans[ch]
                if smp: c["smp"] = smp
                if period: c["period"] = period; c["pos"] = 0.0; c["on"] = True
                if not c["on"] or not c["smp"]: continue
                name, data, loop, vol = SAMPLES[c["smp"] - 1]
                step = PAL_CLOCK / c["period"] / rate
                cnt = t1 - t0
                idx = c["pos"] + np.arange(cnt) * step
                if loop:
                    ls, ll = loop
                    idx = np.where(idx >= ls + ll, ls + (idx - ls) % ll, idx)
                    seg = data[np.minimum(idx.astype(int), len(data) - 1)]
                    c["pos"] = float(idx[-1] + step)
                else:
                    ii = idx.astype(int)
                    seg = np.where(ii < len(data), data[np.minimum(ii, len(data) - 1)], 0.0)
                    c["pos"] = float(idx[-1] + step)
                    if c["pos"] >= len(data): c["on"] = False
                seg = seg * (vol / 64.0) * 0.28
                mix[t0:t1, 0] += seg * (1 - pan[ch]) * 2
                mix[t0:t1, 1] += seg * pan[ch] * 2
    # a touch of low-pass, as the Amiga's filter
    k = np.array([0.25, 0.5, 0.25])
    for c in range(2): mix[:, c] = np.convolve(mix[:, c], k, mode="same")
    peak = np.max(np.abs(mix)) or 1.0
    return (mix / peak * 0.9), n / rate

# ---- the Boing ball ------------------------------------------------------------------------------------
def boing_frame(W, H, t, sx=None):
    """One frame as RGB uint8: a red and white checked ball bouncing on a grey grid, as the demo. t in seconds."""
    sx = sx or W
    y_img, x_img = np.mgrid[0:H, 0:W].astype(np.float64)
    out = np.zeros((H, W, 3), dtype=np.uint8)
    out[:] = (170, 170, 170)
    # the grid behind: perspective lines in purple
    gy = (y_img / H)
    horizon = 0.72
    for gx in np.linspace(0, 1, 13):
        xl = W * (0.5 + (gx - 0.5) * (0.35 + 0.9 * gy))
        out[np.abs(x_img - xl) < 0.8] = (150, 70, 170)
    for row_v in np.linspace(0.05, horizon, 10):
        out[np.abs(gy - row_v) < 0.006] = (150, 70, 170)
    # the ball
    R = min(W, H) * 0.22
    cx = W * (0.5 + 0.34 * math.sin(t * 1.3))
    bounce = abs(math.sin(t * 2.6))
    cy = H * (0.62 - 0.36 * bounce)
    # shadow
    sh = ((x_img - cx - R * 0.5) ** 2 / (R * 1.15) ** 2 + (y_img - H * 0.66) ** 2 / (R * 0.35) ** 2) < 1
    out[sh] = (110, 110, 110)
    dx, dy = (x_img - cx) / R, (y_img - cy) / R
    d2 = dx * dx + dy * dy
    inside = d2 < 1
    dz = np.sqrt(np.clip(1 - d2, 0, 1))
    # the ball spins: longitude from x/z, latitude from y; the checks 8 x 8 tilted a little
    tilt = 0.3
    yy = dy * math.cos(tilt) - dz * math.sin(tilt)
    zz = dy * math.sin(tilt) + dz * math.cos(tilt)
    lon = np.arctan2(dx, zz) + t * 2.2 * (1 if math.cos(t * 1.3) > 0 else -1)
    lat = np.arcsin(np.clip(yy, -1, 1))
    check = ((np.floor(lon / (math.pi / 4)) + np.floor((lat + math.pi / 2) / (math.pi / 8))) % 2) == 0
    shade = (0.55 + 0.45 * np.clip(dz * 0.8 + 0.2 - dy * 0.3, 0, 1))
    red = np.stack([230 * shade, 30 * shade, 40 * shade], axis=-1)
    white = np.stack([245 * shade, 245 * shade, 245 * shade], axis=-1)
    ball = np.where(check[..., None], red, white)
    out[inside] = np.clip(ball[inside], 0, 255).astype(np.uint8)
    return out

def write_frames(dirpath, W, H, fps, seconds):
    from PIL import Image
    n = int(fps * seconds)
    for i in range(n):
        Image.fromarray(boing_frame(W, H, i / fps)).save(dirpath / f"f{i:05d}.png")
    return n

# ---- CDXL ------------------------------------------------------------------------------------------------
def to_amiga_palette(frames_rgb, colours=32):
    """One palette for the film (median cut by PIL), and each frame as indexes."""
    from PIL import Image
    big = Image.fromarray(np.concatenate(frames_rgb[::max(1, len(frames_rgb) // 8)], axis=0))
    pal_img = big.quantize(colors=colours, method=Image.Quantize.MEDIANCUT)
    pal = pal_img.getpalette()[:colours * 3]
    idx_frames = [np.array(Image.fromarray(f).quantize(palette=pal_img, dither=Image.Dither.NONE)) for f in frames_rgb]
    return pal, idx_frames

def planar(idx, planes):
    """Index image (H, W) -> bitplanes, each plane the whole frame, rows padded to 16 pixels."""
    H, W = idx.shape
    Wp = (W + 15) // 16 * 16
    padded = np.zeros((H, Wp), dtype=np.uint8); padded[:, :W] = idx
    out = bytearray()
    for p in range(planes):
        bits = (padded >> p) & 1
        out += np.packbits(bits, axis=1).tobytes()
    return bytes(out)

def write_cdxl(path, frames_rgb, audio_mono_8bit, fps, rate):
    pal, idx_frames = to_amiga_palette(frames_rgb, 32)
    planes = 5
    pal_bytes = b"".join(struct.pack(">H", ((pal[i * 3] >> 4) << 8) | ((pal[i * 3 + 1] >> 4) << 4) | (pal[i * 3 + 2] >> 4))
                         for i in range(32))
    H, W = idx_frames[0].shape
    per_frame = rate // fps
    if per_frame % 2: per_frame += 1
    out = bytearray()
    prev = 0
    for i, idx in enumerate(idx_frames):
        video = planar(idx, planes)
        a0 = i * per_frame
        aud = audio_mono_8bit[a0:a0 + per_frame].tobytes().ljust(per_frame, b"\0")
        size = 32 + len(pal_bytes) + len(video) + len(aud)
        # type 1 custom, format: bit planar, mono 8-bit; chunk size, previous size, frame number; w, h, planes,
        # palette bytes, audio bytes, frequency, fps, pad
        hdr = struct.pack(">BBIIIHHHHHHBB", 1, 0x00, size, prev, i + 1, W, H, planes, len(pal_bytes), len(aud), rate, fps, 0)
        hdr = hdr.ljust(32, b"\0")
        out += hdr + pal_bytes + video + aud
        prev = size
    Path(path).write_bytes(bytes(out))
    return len(out), len(idx_frames)

# ---- main ---------------------------------------------------------------------------------------------
def main():
    out = Path(sys.argv[1] if len(sys.argv) > 1 else "samples")
    out.mkdir(parents=True, exist_ok=True)
    rate = 44100
    print("Boing.mod:", write_mod(out / "Boing.mod"), "bytes")
    mix, seconds = render(rate)
    pcm = (mix * 32767).astype("<i2").tobytes()
    tmp = Path(tempfile.mkdtemp(prefix="boing-"))
    (tmp / "tune.raw").write_bytes(pcm)
    aud = ["-f", "s16le", "-ar", str(rate), "-ac", "2", "-i", str(tmp / "tune.raw")]
    subprocess.run(["ffmpeg", "-hide_banner", "-loglevel", "error", "-y", *aud, "-codec:a", "libmp3lame", "-b:a", "160k",
                    "-metadata", "title=Boing", "-metadata", "artist=The AmigaChrome Team", str(out / "Boing.mp3")], check=True)
    print("Boing.mp3: %.1f s" % seconds)
    # the film, 320 x 180 at 24 fps, as long as the tune
    frames_dir = tmp / "frames"; frames_dir.mkdir()
    n = write_frames(frames_dir, 320, 180, 24, seconds)
    subprocess.run(["ffmpeg", "-hide_banner", "-loglevel", "error", "-y", "-framerate", "24", "-i", str(frames_dir / "f%05d.png"),
                    *aud, "-shortest", "-codec:v", "libx264", "-profile:v", "baseline", "-level", "3.0", "-pix_fmt", "yuv420p",
                    "-crf", "22", "-codec:a", "aac", "-b:a", "128k", "-movflags", "+faststart",
                    "-metadata", "title=Boing", str(out / "Boing.mp4")], check=True)
    print("Boing.mp4:", n, "frames")
    # the CDXL: 160 x 100 at 12 fps, mono 8-bit at 11025 Hz
    cd_fps, cd_rate = 12, 11025
    frames = [boing_frame(160, 100, i / cd_fps) for i in range(int(cd_fps * min(seconds, 40)))]
    mono = mix.mean(axis=1)
    mono_rs = np.interp(np.arange(0, len(mono), rate / cd_rate), np.arange(len(mono)), mono)
    mono8 = np.clip(mono_rs * 127, -128, 127).astype(np.int8)
    size, nf = write_cdxl(out / "Boing.cdxl", frames, mono8, cd_fps, cd_rate)
    print("Boing.cdxl:", nf, "frames,", size, "bytes")
    # check it with ffmpeg's CDXL demuxer
    r = subprocess.run(["ffprobe", "-hide_banner", "-v", "error", "-show_entries", "stream=codec_name,width,height,sample_rate",
                        "-of", "csv=p=0", str(out / "Boing.cdxl")], capture_output=True, text=True)
    print("  ffprobe:", r.stdout.strip().replace("\n", "; ") or r.stderr.strip())
    r = subprocess.run(["ffmpeg", "-hide_banner", "-loglevel", "error", "-y", "-i", str(out / "Boing.cdxl"), "-frames:v", "1",
                        str(tmp / "cdxl_check.png")], capture_output=True, text=True)
    print("  decoded a frame:", "ok" if r.returncode == 0 else r.stderr.strip())
    (out / "README.txt").write_text(
        "Sample media for OpenPlay, made by the AmigaChrome Team (MIT): a chiptune, Boing, and the Boing ball bouncing to it.\n"
        "  Boing.mod   ProTracker module, 4 channels\n  Boing.mp3   the tune rendered\n"
        "  Boing.mp4   the film, 320 x 180, H.264 + AAC\n  Boing.cdxl  the film for the chipset: CDXL 160 x 100, 32 colours, 12 fps\n"
        "Made by openamigamediaplayer/tools/make_samples.py.\n")

if __name__ == "__main__":
    main()
