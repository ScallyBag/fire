# Fire

![alt tag](https://raw.githubusercontent.com/FireFather/fire/master/docs/fire.png)

a very strong, state-of-the-art, highly optimized, open-source freeware UCI chess engine...
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
- integrated tuner (if compiled with #define TUNER)
- timestamped bench, perft/divide, and tuner logs
- asychronous cout (acout) class using std::unique_lock<std::mutex>
- uci option searchtype random w/ uniform_real_distribution & mesenne_twister_engine
- reads engine.conf on startup for search, eval, pawn, and material parameters
- NNUE support

Fire 8 has undergone months of meticulous analysis and refactoring using many of the most modern C++ tools available today, including Clang, ReSharper C++, and Visual Studio Code Analysis, ensuring the production of extremely fast highly optimized and stable executables.

**Fire 8.N (NNUE) is now available**

## strength
Fire 8 has been thoroughly tested by the CCRL testing group:
- ~3550 Elo on [CCRL Blitz](https://www.computerchess.org.uk/ccrl/404/)
- ~3400 Elo on [CCRL 40/15](https://www.computerchess.org.uk/ccrl/4040/)
- ~3600 Elo on [CCRL 40/2 FRC](https://www.computerchess.org.uk/ccrl/404FRC/)

Fire appears to be superior to Ethereal and really shines at ultra-fast TCs like 1000ms + 100 ms (avg. game 14 secs)


| engine         | games    | win      | loss     | draw      | timeouts  | win%      | elo      | los        
| :------------: | :------: | :------: | :------: | :------:  | :------:  | :------:  | :------: | :------:  
| Fire 8.1       | 5697     | +2334    | -1221    | =2142     | 0         | 59.8%     | +70 elo  | 100%
| Ethereal 2.75  | 5697     | +1221    | -2334    | =2142     | 0         | 40.2%     | -70 elo  | 0%


60 secs + 0.6 secs: http://www.fastgm.de/60-0.60.html

10 min + 6 secs: http://www.fastgm.de/10min.html


## available binaries
- **x64 popc** = fast pgo binary (for modern 64-bit systems w/ popcnt instruction set)
- **x64 avx2** = fast pgo binary (for modern 64-bit systems w/ AVX2 instruction set)
- **x64 bmi2** = fast pgo binary (for modern 64-bit systems w/ BMI2 instruction set)

- **windows** : Fire_8.N_x64_bmi2.exe, Fire_8.N_x64_avx2.exe, or Fire_8.N_x64_popc.exe
- **linux** :   Fire_8.N_x64_avx2 or Fire_8.N_x64_popc

You can download a strong NNUE net to use with Fire 8.N from many places...here are a couple sites with many to offer:
- https://www.comp.nus.edu.sg/~sergio-v/nnue/
- https://tests.stockfishchess.org/nns/
(rename the downloaded net -> nn.bin and place it in the same directory as the Fire_8.N_x64 executable.)

Be aware that, due to lack of avx2 instruction set, the popc binaries are considerably slower than the bmi2 and/or avx2 binaries.

Here is a complete list of recommended processors for Fire 8.N x64:

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

run 'bench' at command line to determine which binary runs best/fastest on your system. for greater accuracy, run it twice and calculate the average of both results.


please see **http://chesslogik.wix.com/fire** for more info

## compile it yourself
- **windows** (visual studio) use included project files: Fire.vcxproj or Fire.sln
- **minGW** run one of the included bash shell scripts: makefire_pext.sh or makefire_popc.sh
- **ubuntu** type 'make profile-build ARCH=x86-64-pext' or 'make profile-build ARCH=x86-64-popc'

## uci options
- **Hash** size of the hash table. default is 64 MB.
- **Threads** number of processor threads to use. default is 1, max = 128.
- **MultiPV** number of pv's/principal variations (lines of play) to be output. default is 1.
- **Contempt** higher contempt resists draws.
- **Ponder** also think during opponent's time. default is false.
- **UCI_Chess960** play chess960 (often called FRC or Fischer Random Chess). default is false.
- **Clear Hash** clear the hash table. delete allocated memory and re-initialize.
- **SyzygyProbeDepth** engine begins probing at specified depth. increasing this option makes the engine probe less.
- **EngineMode** choose hybrid (default), classic, NNUE, or random.
- **SyzygyProbeLimit** number of pieces that have to be on the board in the endgame before the table-bases are probed.
- **Syzygy50MoveRule** set to false, tablebase positions that are drawn by the 50-move rule will count as a win or loss.
- **SyzygyPath** path to the syzygy tablebase files.


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

the endgame table bases are implemented using code adapted from
- [Syzygy EGTBs & probing code](https://github.com/syzygy1/tb)

the quiescent search positions for the tuner have been generously shared by
- [Zurichess](https://bitbucket.org/zurichess/zurichess/src/master/)

if you are interested in learning about my particular testing methodology, I've explained it in some detail here:
http://www.chessdom.com/fire-the-chess-engine-releases-a-new-version/


## license
Fire is distributed under the GNU General Public License. Please read LICENSE for more information.


best regards-

Norm

firefather@telenet.be
