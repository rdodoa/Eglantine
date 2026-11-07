<!--
SPDX-FileCopyrightText: Copyright 2026 Eden Emulator Project
SPDX-License-Identifier: GPL-3.0-or-later
-->

<p align="center">
  <img src="dist/branding/hortensia_icon.png" alt="Hortensia" width="180">
</p>

<h1 align="center">Hortensia</h1>

<p align="center">
  <b>An experimental Nintendo Switch 2 emulator that gathers the best work of the Eglantine forks in one build.</b><br>
  Windows · Linux · macOS
</p>

---

## Contents

- [What is Hortensia?](#what-is-hortensia)
- [Download](#download)
- [Setting it up](#setting-it-up)
- [Optimized settings per game](#optimized-settings-per-game)
- [Recommended settings per game](#recommended-settings-per-game)
- [What's inside, fork by fork](#whats-inside-fork-by-fork)
- [Every switch Hortensia adds](#every-switch-hortensia-adds)
- [What was left out, and why](#what-was-left-out-and-why)
- [Reporting a problem](#reporting-a-problem)
- [Known issues](#known-issues)
- [Building from source](#building-from-source)
- [Credits](#credits)

## What is Hortensia?

The Switch 2 emulation scene moves fast and is spread over many forks of
[Eglantine](https://github.com/Djihads80/Eglantine): one is best for Mario Kart World, another
for Ocarina of Time, another for AMD graphics cards… Hortensia picks the best changes from each of
them and puts them in a single emulator, so you don't have to juggle five builds.

Hortensia starts from **[Lichii](https://github.com/zortos293/Lichii)** (`nx2`, the most active
fork, with the best Ocarina of Time support) and adds work from the forks listed below. **Every
change keeps its original author** in the git history, and every optimization Hortensia adds can be
switched off (see [Every switch Hortensia adds](#every-switch-hortensia-adds)).

> [!WARNING]
> This is experimental software. Games may crash, show graphical errors or run slowly.
> You need your own legally dumped games, keys and firmware. Hortensia ships none of them.

## Download

Get the latest build from the **[Releases page](https://github.com/justr4mzi/hortensia/releases)**.
There is **one file per system**:

| Your computer | Download |
| --- | --- |
| **Windows** (almost every PC) | **`Hortensia-Windows.zip`** |
| Windows, only if Hortensia doesn't start or shows graphical errors | `Hortensia-Windows-Compatibility.zip` |
| **Linux** | **`Hortensia-Linux.AppImage`** (`Hortensia-Linux-ARM.AppImage` on ARM) |
| **Mac** with Apple Silicon (M1 or newer) | **`Hortensia-macOS.zip`** |

<details><summary>What's the difference between the two Windows files?</summary>

- `Hortensia-Windows.zip` is compiled with clang, a performance profile (PGO) and instructions of
  CPUs from 2013 on (AVX2, x86-64-v3), like Selevet's tuned build: the fastest.
- `Hortensia-Windows-Compatibility.zip` is compiled with Microsoft's compiler for any 64-bit CPU.
  Use it only if the first one doesn't start (very old CPU) or draws something wrong.

</details>

## Setting it up

1. **Extract** the zip into a folder (right-click → *Extract All*), for example `C:\Games\Hortensia`,
   and double-click `hortensia.exe`. If Windows says it protected your PC, click **More info →
   Run anyway** (Hortensia isn't signed).
   - Linux: `chmod +x Hortensia-Linux.AppImage`, then run it.
   - Mac: open the zip, drag Hortensia to Applications, right-click it → **Open** the first time.
2. **First start.** Hortensia does a few things for you:
   - If you used **Lichii** or **Eden/Eglantine** before, it offers to **import** your keys,
     firmware, saves, settings and **shader caches** (no stutter in games you already played).
   - It picks your **dedicated graphics card** over the processor's integrated graphics.
   - It asks about **optimized settings**: answer **Yes** (see below).
   - It switches to the **Hortensia theme** (Emulation → Configure → General → UI to change it).
3. **Keys and firmware** (skip this if you imported them). Hortensia keeps its own data folder, so
   it never touches the config of your other emulators:
   - Windows: `%APPDATA%\hortensia` (type it in the Explorer address bar)
   - Linux: `~/.local/share/hortensia` and `~/.config/hortensia`
   - macOS: `~/Library/Application Support/hortensia`

   Copy your `keys` folder there and install your firmware from the **File** menu, exactly as you
   did for Eglantine, Lichii or Eden.
   **Portable mode:** create an empty folder named `user` next to `hortensia.exe` and Hortensia keeps
   everything in it instead.
4. **Add your games folder** (double-click the game list). Each game shows **Switch 1** or
   **Switch 2** under its name. The first launch of a game builds its shaders: it stutters for a
   few minutes, then gets smooth.

### Using someone else's shader cache

Hortensia reads Lichii's cache files (`vulkan.fork-v53.bin`). Put a game's cache folder (named after
its program ID, for example `0400486024dec000` for Ocarina of Time) in
`%APPDATA%\hortensia\shader\` on Windows. Only the `vulkan.fork-v53.bin` and
`vulkan_used.fork-v53.bin` files matter; files from other builds (`-frizti.fork-v52`, …) are
ignored. The guest shader list works on any graphics card; the driver's own cache is rebuilt for
yours.

## Optimized settings per game

At its first start, Hortensia asks whether it should **set the best settings for each game
automatically**. With the option on (it can be changed any time in **Emulation → Optimized
Settings per Game**), starting a supported game applies its settings for that session only:
your saved settings are never changed, and the log says what was applied
(`Hortensia optimized settings (AMD GPU): …`).

Hortensia reads your **graphics card's vendor** from the selected Vulkan device:

- **NVIDIA and AMD** (dedicated cards): 1x resolution, uncompressed ASTC textures in Mario Kart
  World.
- **Intel, Apple and Qualcomm** (integrated GPUs): half resolution, BC3-compressed textures.

If a game shows a black screen entering a race or a level with these settings, turn the option
off for that game (Selevet saw it with memory operation syncs off on an older build).

| Game | What is applied |
| --- | --- |
| Mario Kart World | Lichii's recommended settings (Vulkan, GPU accuracy Fast, asynchronous shaders, conservative VRAM, reactive flushing off, FIFO Relaxed VSync, handheld mode, ScaleForce, FXAA), **dynamic FPS on**, plus Selevet's tuned settings (CPU and DMA accuracy Unsafe, Immediate GPU fences, memory operation syncs off), ASTC as above |
| Ocarina of Time | Vulkan, asynchronous shaders (the Max Resolution and Dynamic FPS patches are on anyway) |
| Donkey Kong Bananza | Vulkan, asynchronous shaders, dynamic FPS, plus Orchid's settings measured on AMD: GPU accuracy Fast, reactive flushing off, Boost GPU clock (it then stays at 1080p and mostly 60 fps pacing) |
| Pokémon Pokopia | Vulkan, asynchronous shaders |
| Splatoon Raiders | Vulkan, asynchronous shaders |

With the option off, Hortensia behaves like Lichii (it asks about Mario Kart World's settings once
per session).

## Recommended settings per game

Each tip below names **where it comes from and on which PC it was measured.** These are the
measurements published by the fork authors, not tests of Hortensia itself: your results will
differ with your hardware and driver. Settings live in **Emulation → Configure**.

### General (every game)

| Setting | Value | Why |
| --- | --- | --- |
| Graphics → API | Vulkan | The only backend the Switch 2 path is built for |
| Graphics → Advanced → Enable asynchronous shader compilation | On | Fewer pauses while shaders build |
| CPU → Accuracy | Auto (or Unsafe for more speed) | Selevet's tuned build uses Unsafe |

### Mario Kart World

| Setting | Value |
| --- | --- |
| Graphics → Advanced → **Mario Kart World: dynamic FPS (experimental)** | On (restart the game after changing it) |
| Resolution | 1x (0.5x on weaker GPUs) |

- Hortensia holds a new swap interval back for 120 presents in Mario Kart World
  (`EDEN_NX2_SWAP_STABLE`): without it the game flips between 60 and 30 fps pacing and fell from
  about 40 to 20-30 fps.
- **Measured by Selevet** on a Ryzen 5 5500, RTX 3050 8 GB, 16 GB RAM: **55.5 fps median** at
  1x, lowest 600-frame average 25.9 fps (Selevet's own tuned build). Heavy tracks still drop while
  textures load: this is not a locked 60.
- The dynamic FPS fix (the setting did nothing before) and the cheaper ASTC texture decode come
  from **SmileyTatsu**.

### The Legend of Zelda: Ocarina of Time

| Setting | Value |
| --- | --- |
| Add-Ons → **Max Resolution (built-in)** | On (default) |
| Add-Ons → **Dynamic FPS (built-in)** | On (default): the game keeps its normal speed below 60 fps |
| `nx2_switches.txt` → `EDEN_NX2_OOT_VISUAL_WORKAROUND` | On in Hortensia's file (less fog haze and glare) |

- **Measured by zortos293 (Lichii)** in Kokiri Forest on an i7-12700K + **RX 7800 XT (AMD)**:
  25.97 → **30.75 fps** average with the GPU-thread optimizations Hortensia includes.
- **Measured by Selevet** on a Ryzen 5 5500 + RTX 3050: about 26-28 fps in Kokiri Forest. The
  emulator's GPU threads are the limit, not the graphics card.
- If the game crashes on an AMD card, try `EDEN_NX2_GPU_TIME_DIV=4` (zortos293's "K4" test) in
  your `nx2_switches.txt`, or skip the compute shader named in the log (`EDEN_NX2_SKIP_COMPUTE`).

### Donkey Kong Bananza

| Setting | Value |
| --- | --- |
| Graphics → Advanced → **Donkey Kong Bananza: dynamic FPS (experimental)** | On (default) |
| Frame generation | Optional, toggle in game with **Ctrl+G** |

- Dynamic FPS (from **ZendNevs**, via Eglantine-DKB): the game follows real time instead of
  slowing down when the frame rate drops.
- **Measured by MythosOfs (Orchid 0.6.1)** on AMD with RADV (Linux): the first area went from
  about 22 to 59 fps. Later areas are still slow.

### Pokémon Pokopia

- Works out of the box. Hortensia doesn't pace storage reads for Switch 2 games (from
  **notsapinho**), which made Pokopia boot about **13 s faster** in his tests.
- notsapinho measured world frames at 8.4-8.6 ms on his own build (120 Hz mod).

### Splatoon Raiders

- Works out of the box. No fork has published measurements yet.

## What's inside, fork by fork

The full list with authors is in the commit history. Here is what each source brings.

### Base: [Lichii](https://github.com/zortos293/Lichii) `nx2` (zortos293 and contributors)

Everything Lichii had on 6 November 2026 (17:00), including:

- Ocarina of Time: boot, rendering, the built-in **Max Resolution** and **Dynamic FPS** patches,
  door fix, upscaler gathers, white flashes fix, the last compute shader forms.
- The **AMD black-scene fix** (#127, also in Selevet's and Orchid's AMD fixes), **MWAITX/UMWAIT** wait fixes for Ryzen and Intel.
- The **AMD fix of 6 November** (#273): depth-compare textures typed as float, atomic retries
  that use the value the compare-and-swap returned, and two Ocarina compute shaders that took
  6-18 minutes to build on AMD's Windows driver built unoptimized.
- **"perf9"**: 17 GPU-thread optimizations promoted by default (cheaper method dispatch,
  batched constant-buffer writes, page-run cache, SPIR-V emission on pipeline workers, smarter
  waits before MME DMA reads, read-back batching: Ocarina's Finishes went from 1184 to 70 a
  second).
- **Frizti's performance work** (#194): event waits instead of spinning, Ocarina one frame
  ahead, 4096-draw submissions, deferred fragment barriers, cheaper CPU timer reads, uniform bank
  uploads without page protection churn, FSR3 fixes.

### Added on top

| From | Change | Switch |
| --- | --- | --- |
| [Eglantine](https://github.com/Djihads80/Eglantine) (Jonas, Erick, Christian, failaip12, MythosOfs…) | H0_NH1 mixed-sign half source; NVDEC end of stream; pipeline cache recovery; Turing MME register reads; stream buffer on small BAR heaps; x64 call reach; Switch 2 channel state; atomic exit flags; GPU device fallback; memory manager reference counts | — |
| [Selevet](https://github.com/sylvieistaken/selevet-eglantine) | Semaphore acquires already met in memory, or met by a queued release, skip the GPU-thread drain; SEM_EXECUTE without a drain; GPU wait polling; high process priority; VRR reported to the game | `ACQUIRE_PEEK`, `ACQUIRE_QUEUED`, `SEMEXEC_DRAIN`, `GPU_WAIT_SPIN_US`, `HIGH_PRIORITY`, `REPORT_VRR` |
| Selevet | **Timer thread sleeps through long waits** (it kept 91% of a core busy in Mario Kart World, a core the game can use on a 6-core CPU) and is only woken for a new earliest event | `EDEN_TIMING_COARSE`, `EDEN_TIMING_WAKE_ALL` |
| Selevet | CPU float accuracy overrides | `EDEN_CPU_ACCURATE_FLOAT`, `EDEN_CPU_FUSED_FMA`, `EDEN_CPU_GLOBAL_MONITOR` |
| Selevet | First dispatch of each compute shader logged; skip list for GPU hangs | `SKIP_COMPUTE` |
| Selevet | Tunable Ocarina of Time fog/exposure/bloom (on zortos293's workaround) | `OOT_VISUAL_WORKAROUND`, `OOT_FOG_SCALE`, `OOT_EXPOSURE_SCALE`, `OOT_BLOOM_EXP_SCALE` |
| Selevet | Uncompressed ASTC for Mario Kart World (BC3 darkened the ground), used by the optimized settings | — |
| [Orchid](https://github.com/MythosOfs/Orchid) (after leadaenima/Lichii), also in Selevet's AMD fixes | **AMD subgroup shuffle fix**: on AMD drivers (Windows and Linux) graphics shaders that shuffle across lanes no longer lose the upper half of the guest warp on 64-wide hardware; task and mesh shaders left out | — |
| [Orchid](https://github.com/MythosOfs/Orchid) | **Switch 2 graphics services on their own host thread**: vi2's present no longer blocks emulated core 3 for most of every frame | `GSV_HOST_THREAD` |
| Orchid, after Selevet | Mario Kart World swap interval held back for 120 presents (40 → 20 fps drop) | `SWAP_STABLE` |
| [SmileyTatsu](https://github.com/SmileyTatsu/Eglantine) | Mario Kart World dynamic FPS applies (it never did); ASTC data scanned in place and decoded per block (less stutter); guest RAM released when a game stops; Switch 2 pipelines all built at boot and never evicted; no task/mesh pipelines without `VK_EXT_mesh_shader`; crash fix when stopping emulation | — |
| [notsapinho](https://github.com/notsapinho/Eglantine) | Storage reads of Switch 2 games not paced (Pokopia boots ~13 s faster) | — |
| [Eglantine-DKB](https://github.com/monteger8/Eglantine-DKB) (monteger8, adamd), Orchid | Unchanged vertex buffers and repeated written ranges skipped; empty draws skipped; DK Bananza macro DMA reads of untouched pages skip the full drain | `DRAW_TUNING`, `DRAW_EMPTY`, `DMA_DRAIN` |
| ZendNevs (via Eglantine-DKB) | Donkey Kong Bananza dynamic FPS; frame generation toggle in game (Ctrl+G) | `PACE_ONE` |
| Lichii experiments (zortos293) | "K4" GPU time divisor; parallel render pass recording prototype; Ocarina visual workaround | `GPU_TIME_DIV`, `PARALLEL_RECORD`, `OOT_VISUAL_WORKAROUND` |
| [chuppatricky's AMD test builds](https://github.com/chuppatricky/Lichii) (chuppatricky, Matt Artist, sharkiix), v13 | **AMD fixes**: colour targets a fragment shader never writes are masked (dark outlines around Ocarina's castle on AMD); NaN-defined min/max/clamp; misaligned 64/128-bit and vector memory accesses split into words; occlusion query resolves wait once on AMD; compute writes ordered before later draws; sampler comparison dropped for non-depth samples; blend state rebuilt without dynamic blend; four Ocarina fragment shaders and opt-in graphics pipelines built unoptimized on old AMD drivers; query reports written once the GPU produced them; GPU breadcrumbs that name the work running when the GPU hangs, and the memory budget in the log; null texture for handles past the pool; video frames invalidated with GPU writes kept; more shader register fixes; **display frame rate in the status bar** and a **frame generation button** | `EDEN_NX2_AMD_SAFE_COMPILE_GRAPHICS` |
| [Frizti](https://github.com/Frizti/Lichii) | Command list fetch wait skip (experiment, off) | `EDEN_NX2_LIST_WAIT_SKIP` |
| Hortensia | Optimized settings per game with GPU vendor detection; import from Lichii and Eden/Eglantine at the first start (shader caches kept); dedicated GPU picked at the first start; Hortensia theme; Switch 1 / Switch 2 label under each game; Bilinear scaling by default; built-in update check against Hortensia's releases; Lichii's cache format kept so shared caches load; MKW/DK title checks through the present's process; DMA drain skip limited to DK Bananza (Ocarina's fog reads query results this way); conflicts resolved against Lichii's newer code | — |

## Every switch Hortensia adds

Switches are read from `nx2_switches.txt` next to `hortensia.exe`, and from your own copy in the data
folder (`%APPDATA%\hortensia\nx2_switches.txt` on Windows), which wins. One `NAME=VALUE` per line;
restart Hortensia after editing. The file shipped with Hortensia lists them all in its "Hortensia"
section. If something looks wrong after an update, turn the switches off one at a time.

| Switch | Default | Turn it… |
| --- | --- | --- |
| `EDEN_NX2_ACQUIRE_PEEK` | on (off in DK Bananza) | `=0` off |
| `EDEN_NX2_ACQUIRE_QUEUED` | on | `=0` off |
| `EDEN_NX2_SEMEXEC_DRAIN` | off (no drain) | `=1` to drain again |
| `EDEN_NX2_GPU_WAIT_SPIN_US` | 300 | `=0` no polling |
| `EDEN_NX2_HIGH_PRIORITY` | on | `=0` normal priority |
| `EDEN_NX2_REPORT_VRR` | on | `=0` off |
| `EDEN_TIMING_COARSE` | on | `=0` precise waits |
| `EDEN_TIMING_WAKE_ALL` | off | `=1` wake for every event |
| `EDEN_NX2_GSV_HOST_THREAD` | on | `=0` back on emulated core 3 |
| `EDEN_NX2_SWAP_STABLE` | 120 for MKW, 0 otherwise | `=<presents>` for every game |
| `EDEN_NX2_DRAW_TUNING` | on | `=0` off |
| `EDEN_NX2_DRAW_EMPTY` | off (empty draws skipped) | `=1` to draw them |
| `EDEN_NX2_DMA_DRAIN` | off (DK skip on) | `=1` always drain |
| `EDEN_NX2_PACE_ONE` | on (DK) | `=0` keep DK's interval |
| `EDEN_NX2_ASYNC_SKIP` | off | `=1` skip small draws while their pipeline builds |
| `EDEN_NX2_OOT_VISUAL_WORKAROUND` | on in Hortensia's file (Ocarina only) | delete the line |
| `EDEN_NX2_OOT_FOG_SCALE` / `_EXPOSURE_SCALE` / `_BLOOM_EXP_SCALE` | 0.35 / 0.75 / 1.2 | lower = less haze/glare |
| `EDEN_CPU_ACCURATE_FLOAT`, `EDEN_CPU_FUSED_FMA`, `EDEN_CPU_GLOBAL_MONITOR` | off | `=1` on |
| `EDEN_NX2_SKIP_COMPUTE` | empty | `=hash,hash` |
| `EDEN_NX2_GPU_TIME_DIV` | 1 | `=4` (K4 test) |
| `EDEN_NX2_PARALLEL_RECORD` | off | `=1` prototype |
| `EDEN_NX2_AMD_SAFE_COMPILE` | automatic (AMD Windows drivers older than 2.0.388) | `=1` on any AMD Windows driver, `=0` never |
| `EDEN_NX2_AMD_SAFE_COMPILE_GRAPHICS` | off | `=1` builds every graphics pipeline unoptimized on AMD's Windows driver (crash near the Deku Tree) |
| `EDEN_NX2_LIST_WAIT_SKIP` | off | `=1` experiment |

## What was left out, and why

Being honest about the gaps helps when you compare Hortensia with another build.

- **Lichii's "fast draw" branch** (`FAST_DRAW`, `BUFFER_SKIP`, `INLINE_INVALIDATE`,
  `DYNAMIC_SKIP`, `DMA_RANGES`, `LIST_TRUST`), which Selevet's Ocarina builds turn on: it is 181
  commits behind Lichii and conflicts with the perf9 code that replaced much of it. zortos293
  keeps it experimental.
- **Orchid's shader reuse and asynchronous translation** (from zortos293's build 31): Lichii now
  emits SPIR-V on pipeline workers itself (perf9), which covers the same stalls.
- **Jonas's Ocarina optimizations** (idle spin, overlapping-write waits, MaxContinuousRange
  cache): already in Lichii.
- **Orchid's command-list drain skip** (Mario Tennis Fever only) and **SmileyTatsu's single-submit
  ASTC transcode**: they rewrite code Lichii changed since; left for a later version.
- **Selevet's thread pinning** (opt-in only) and **inline CNTPCT** (needs a modified dynarmic).
- **Pokopia FPS-mod settings (120-360 Hz)** from notsapinho: a larger settings change, planned.
- **Frizti's newest split-thread work** (acquire precheck, macro reads served from queued constant
  buffer uploads): it overlaps the Selevet acquire optimizations already in Hortensia and conflicts
  with them; left for a later version. Most of Frizti's earlier work is in Lichii (#194).
- **chuppatricky's Ocarina fog modes** (`EDEN_NX2_OOT_FOG`): Hortensia keeps Selevet's tunable fog,
  exposure and bloom instead. His scheduler change for the split's submits conflicts with
  Lichii's newer code.

## Reporting a problem

Open an [issue](https://github.com/justr4mzi/hortensia/issues) and attach:

- the log file `hortensia_log.txt` (the **Log Folder** entry in the menu bar opens its folder);
- the game and its version, your CPU, your graphics card and its driver version;
- which build you used (the zip name), whether optimized settings are on, and what you changed
  in `nx2_switches.txt`;
- what you did and what went wrong (a screenshot helps a lot).

## Known issues

- **Ocarina of Time** runs at about 30 fps on most PCs: the emulator's GPU threads are the limit.
  It plays at normal speed thanks to the Dynamic FPS patch.
- **AMD graphics cards**: most testing in the scene is done on NVIDIA. Logs from AMD users are
  very welcome.
- **Donkey Kong Bananza**: later areas are still slow.
- The first launch of a game builds its shaders: expect stutter for a few minutes.

## Building from source

Hortensia builds like Eglantine and Lichii: see the [build guide](docs/Build.md). The release
binaries come from [.github/workflows/release-binaries.yml](.github/workflows/release-binaries.yml)
(Windows MSVC, Windows MinGW clang PGO and PGO AVX2, Linux x86_64/aarch64 standard and PGO,
macOS arm64).

## Credits

Hortensia only gathers other people's work. All credit goes to the developers of
[Lichii](https://github.com/zortos293/Lichii) (zortos293, Frizti, Kajcsi, Inc0rporate, Shrxmy and
others), [Eglantine](https://github.com/Djihads80/Eglantine) (Djihads80, Jonas and others),
[Selevet](https://github.com/sylvieistaken/selevet-eglantine),
[SmileyTatsu's fork](https://github.com/SmileyTatsu/Eglantine),
[Orchid](https://github.com/MythosOfs/Orchid) (MythosOfs),
[notsapinho's fork](https://github.com/notsapinho/Eglantine),
[Eglantine-DKB](https://github.com/monteger8/Eglantine-DKB) (monteger8, adamd), ZendNevs,
[Eden](https://git.eden-emu.dev/eden-emu/eden) and yuzu. Please don't report Hortensia bugs to them.

Hortensia is put together by [justr4mzi](https://github.com/justr4mzi).

## License

GPL-3.0-or-later, like Eden and Eglantine. See [LICENSE.txt](LICENSE.txt).

*"Nintendo Switch" is a trademark of Nintendo. Hortensia is not affiliated with Nintendo in any way.
This software should not be used to play games you have not legally obtained.*
