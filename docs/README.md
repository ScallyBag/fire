# Fire

![alt tag](https://raw.githubusercontent.com/FireFather/fire/master/docs/fire.png)

a strong, state-of-the-art, highly optimized, open-source freeware UCI chess engine...
designed and programmed for modern 64-bit windows and linux systems
by Norman Schmidt


## features
- c++17
- windows & linux
- uci
- 64-bit
- smp (to 256 threads)
- configurable hash (to 1024 GB)
- ponder
- multiPV
- analysis (infinite) mode
- chess960 (Fischer Random)
- syzygy tablebases
- adjustable contempt setting
- fast perft & divide
- bench (includes ttd time-to-depth calculation)
- timestamped bench, perft/divide, and tuner logs
- asychronous cout (acout) class using std::unique_lock<std::mutex>
- uci option searchtype random w/ uniform_real_distribution & mesenne_twister_engine
- reads engine.conf on startup for search, eval, pawn, and material parameters
- uses a NNUE evaluation
- fast alpha-beta search
- or optional experimental MCTS-UCT search
 (Monte Carlo Tree Search w/ Upper Confidence Bounds Applied to Trees) pure/no minmax

**Fire 8.NN.MC.3 (NNUE - MCTS/UCT) is now available**

![alt tag](https://raw.githubusercontent.com/FireFather/fire/master/Fire_8.NN.MCx64.PNG)

Fire 8.NN.MC has undergone meticulous analysis and refactoring using many of the most modern C++ tools available today, including Clang, ReSharper C++, and Visual Studio Code Analysis, ensuring the production of extremely fast highly optimized and stable executables.


## available binaries
- **x64 avx2** = fast pgo binary (for modern 64-bit systems w/ AVX2 instruction set)
- **x64 bmi2** = fast pgo binary (for modern 64-bit systems w/ BMI2 instruction set)
- **x64 popc** = fast pgo binary (for modern 64-bit systems w/ popcount instruction set)

- **windows** : Fire_8.NN.MC.3_x64_bmi2.exe, Fire_8.NN.MC.3_x64_avx2.exe, Fire_8.NN.MC.3_x64_popc.exe
- **linux** :   Fire_8.NN.MC.3_x64_bmi2, Fire_8.NN.MC.3_x64_avx2, Fire_8.NN.MC.3_x64_popc

Be aware that, due to lack of avx2 instruction set, the popc binaries are much much slower than the bmi2 and/or avx2 binaries.

Here is a complete list of recommended processors for Fire 8.NN.MC x64:

**AVX2 (Advanced Vector Extensions 2)** (also known as Haswell New Instructions)

Intel
- Haswell processor (only Core and Xeon branded), Q2 2013
- Haswell E processor, Q3 2014
- Broadwell processor, Q4 2014
- Broadwell E processor, Q3 2016
- Skylake processor (only Core and Xeon branded), Q3 2015
- Kaby Lake processor (only Core and Xeon branded), Q3 2016 (ULV mobile)/Q1 2017 (desktop/mobile)
- Skylake-X processor, Q2 2017
- Coffee Lake processor (only Core and Xeon branded), Q4 2017
- Cannon Lake processor, Q2 2018
- Cascade Lake processor, Q2 2019
- Ice Lake processor, Q3 2019
- Comet Lake processor (only Core and Xeon branded), Q3 2019
- Tiger Lake (Core, Pentium and Celeron branded[11]) processor, Q3 2020
- Rocket Lake processor, 2021
- Alder Lake processor, 2021
- Gracemont processors, 2021

AMD
- Excavator processor and newer, Q2 2015
- Zen processor, Q1 2017
- Zen+ processor, Q2 2018
- Zen 2 processor, Q3 2019
- Zen 3 processor, 2020
VIA:
- Nano QuadCore
- Eden X4

**BMI2 (Bit Manipulation Instruction Set 2)**

Intel
- Intel Nehalem processors and newer (like Sandy Bridge, Ivy Bridge) (POPCNT supported)
- Intel Silvermont processors (POPCNT supported)
- Intel Haswell processors and newer (like Skylake, Broadwell) (ABM, BMI1 and BMI2 supported)[6]

AMD
K10-based processors (ABM supported)
- "Cat" low-power processors
- Bobcat-based processors (ABM supported)[14]
- Jaguar-based processors and newer (ABM and BMI1 supported)[4]
- Puma-based processors and newer (ABM and BMI1 supported)[4]
"Heavy Equipment" processors
- Bulldozer-based processors (ABM supported)
- Piledriver-based processors (ABM, BMI1 and TBM supported)[1]
- Steamroller-based processors (ABM, BMI1 and TBM supported)
- Excavator-based processors and newer (ABM, BMI1, BMI2 and TBM supported; microcoded PEXT and PDEP)[9]
- Zen-based, Zen+-based, and processors (ABM, BMI1 and BMI2 supported; microcoded PEXT and PDEP)
- Zen 3 processors and newer (ABM, BMI1 and BMI2 supported; full hardware implementation)

Run 'bench' at command line to determine which binary runs best/fastest on your system. for greater accuracy, run it twice and calculate the average of both results.


please see **http://chesslogik.wix.com/fire** for more info

## compile it yourself
- **windows** (visual studio) use included project files: Fire.vcxproj or Fire.sln
- **minGW** run one of the included bash shell scripts: makefire_bmi2.sh, makefire_axv2.sh or makefire_popc.sh
- **ubuntu** type 'make profile-build ARCH=x86-64-bmi2' or 'make profile-build ARCH=x86-64-avx2' or 'make profile-build ARCH=x86-64-popc'

## uci options
- **Hash** size of the hash table. default is 64 MB.
- **Threads** number of processor threads to use. default is 1, max = 128.
- **MultiPV** number of pv's/principal variations (lines of play) to be output. default is 1.
- **Contempt** higher contempt resists draws.
- **MoveOverhead** Adjust this to compensate for network and GUI latency. This is useful to avoid losses on time.
- **MinimumTime** Absolute minimum time (in ms) to spend on a single move. Also useful to avoid losses on time.
- **Ponder** also think during opponent's time. default is false.
- **UCI_Chess960** play chess960 (often called FRC or Fischer Random Chess). default is false.
- **Clear Hash** clear the hash table. delete allocated memory and re-initialize.
- **SyzygyProbeDepth** engine begins probing at specified depth. increasing this option makes the engine probe less.
- **EngineMode** choose NNUE (default), or random.
- **MCTS** enable Monte Carlo Tree Search w/UCT (Upper Confidence Bounds Applied to Trees)
- **SyzygyProbeLimit** number of pieces that have to be on the board in the endgame before the table-bases are probed.
- **Syzygy50MoveRule** set to false, tablebase positions that are drawn by the 50-move rule will count as a win or loss.
- **SyzygyPath** path to the syzygy tablebase files.
- **NnueEvalFile** path to the NNUE evaluation file.

## acknowledgements
many of the ideas & techiques incorporated into Fire are documented in detail here
- [Chess Programming Wiki](https://www.chessprogramming.org)

and some have been adapted from the super strong open-source chess engine
- [Stockfish](https://github.com/official-stockfish/Stockfish)

and others
- [Robbolito](https://github.com/FireFather/robbolito)
- [Ivanhoe](https://www.chessprogramming.org/IvanHoe) [Ivanhoe](http://users.telenet.be/chesslogik/ivanhoe.htm)
- [Houdini](https://www.cruxis.com/chess/houdini.htm)
- [Gull](https://github.com/FireFather/seagull)

the endgame table bases are implemented using code adapted from Ronald de Man's
- [Syzygy EGTBs & probing code](https://github.com/syzygy1/tb)

The NNUN implementation utilizes a modified version of Daniel Shaw's/Cfish excellent nnue probe code:
- [nnue-probe](https://github.com/dshawul/nnue-probe/)

Fire includes 'Raptor', a top reinforcement learning network trained by Sergio Vieri
- https://www.comp.nus.edu.sg/~sergio-v/nnue/
(rename any downloaded net -> nn.bin and place it in the same directory as the Fire_8.NN.MC.2_x64 executable.)

the MCTS implementation is derived from Stephane Nicolet's work
- https://github.com/snicolet/Stockfish/commits/montecarlo

the quiescent search positions for the tuner have been generously shared by
- [Zurichess](https://bitbucket.org/zurichess/zurichess/src/master/)

if you are interested in learning about my particular testing methodology, I've explained it in some detail here:
http://www.chessdom.com/fire-the-chess-engine-releases-a-new-version/


## license
Fire is distributed under the GNU General Public License. Please read LICENSE for more information.


best regards-

Norm

firefather@telenet.be
