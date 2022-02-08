/*
  Fire is a freeware UCI chess playing engine authored by Norman Schmidt.

  Fire utilizes many state-of-the-art chess programming ideas and techniques
  which have been documented in detail at https://www.chessprogramming.org/
  and demonstrated via the very strong open-source chess engine Stockfish...
  https://github.com/official-stockfish/Stockfish.
  
  Fire is free software: you can redistribute it and/or modify it under the
  terms of the GNU General Public License as published by the Free Software
  Foundation, either version 3 of the License, or any later version.

  You should have received a copy of the GNU General Public License with
  this program: copying.txt.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "uci.h"

#include <iostream>
#include <sstream>
#include <string>

#include "bitboard.h"
#include "evaluate.h"
#include "fire.h"
#include "hash.h"
#include "random/random.h"
#include "search.h"
#include "thread.h"
#include "util/perft.h"
#include "util/util.h"
#ifdef TUNER
#include "tune.h"
#endif

// stop threads, reset search
void new_game()
{
	search::signals.stop_analyzing = true;
	thread_pool.main()->wake(false);
	thread_pool.main()->wait_for_search_to_end();
	search::reset();
}

// initialize system
void init(const int hash_size)
{
	thread_pool.start = now();
	bitboard::init();
	position::init();
	search::init();
	evaluate::init();
	pawn::init();
	thread_pool.init();
	search::reset();
	main_hash.init(hash_size);
}

// create infinite loop while parsing for UCI input stream tokens (words)
void uci_loop(const int argc, char* argv[])
{
	position pos{};
	std::string token, cmd;

	pos.set(startpos, uci_chess960, thread_pool.main());
	new_game();

	for (auto i = 1; i < argc; ++i)
		cmd += std::string(argv[i]) + " ";

	do
	{
		// read cmd 'bench' if present, for automated external PGO compile
		if (argc == 1 && !getline(std::cin, cmd))
			cmd = "quit";
		cmd = trim(cmd);
		if (cmd.empty())
			continue;

		std::istringstream is(cmd);

		token.clear();
		is >> std::skipws >> token;
		
		// check for significant UCI tokens
		if (token == "uci")
		{
			acout() << "id name " << program << " " << version << " " << platform << " " << bmis << std::endl;
			acout() << "id author " << author << std::endl;
			acout() << "option name Hash type spin default 64 min 16 max 1048576" << std::endl;
			acout() << "option name Threads type spin default 1 min 1 max 128" << std::endl;
			acout() << "option name MultiPV type spin default 1 min 1 max 64" << std::endl;
			acout() << "option name Contempt type spin default 0 min -100 max 100" << std::endl;	
			acout() << "option name SyzygyProbeDepth type spin default 1 min 0 max 64" << std::endl;
			acout() << "option name SyzygyProbeLimit type spin default 6 min 0 max 6" << std::endl;
			acout() << "option name SearchType type combo default alphabeta var alphabeta var random" << std::endl;
			
			acout() << "option name Ponder type check default false" << std::endl;
			acout() << "option name UCI_Chess960 type check default false" << std::endl;
			acout() << "option name ClearHash type button" << std::endl;			
			acout() << "option name Syzygy50MoveRule type check default true" << std::endl;
			acout() << "option name SyzygyPath type string default <empty>" << std::endl;

			acout() << "uciok" << std::endl;
		}
		else if (token == "isready")
		{
			acout() << "readyok" << std::endl;
		}
		else if (token == "ucinewgame")
		{
			new_game();
		}
		else if (token == "setoption")
		{
			set_option(is);
		}
		else if (token == "position")
		{
			set_position(pos, is);
		}
		else if (token == "go")
		{
			go(pos, is);
		}
		else if (token == "stop")
		{
			search::signals.stop_analyzing = true;
			thread_pool.main()->wake(false);
		}
		else if (token == "quit")
		{
			break;
		}
		else if (token == "perft" || token == "divide")
		{
			int perft_type;
			if (token == "perft")
				perft_type = 1;
			else
				perft_type = 2;
			
			// set params or use default values
			auto depth = is >> token ? token : "7";
			auto hash = is >> token ? token : "64";
			auto threads = is >> token ? token : "1";

			//parse fen words from command line and assign default values if missing
			auto piece_placement = is >> token ? token : "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR";
			auto side_to_move = is >> token ? token : "-";
			auto castling = is >> token ? token : "-";
			auto en_passant = is >> token ? token : "-";
			auto half_moves = is >> token ? token : "";
			auto full_moves = is >> token ? token : "";
			auto fen = piece_placement + " " + side_to_move + " " + castling + " " + en_passant + " " + half_moves + " " + full_moves;

			if (piece_placement == "perft.epd")
				fen = "perft.epd";

			if (perft_type == 1)
				// strings must be converted to integers
				perft(stoi(depth), fen);
			else
				divide(stoi(depth), fen);
		}
		else if (token == "bench")
		{	//bench depth = 16 unless specified on command line
			auto bench_depth = is >> token ? token : "16";	
			// to suppress extraneous output			
			bench_active = true;
			bench(stoi(bench_depth));
			bench_active = false;
		}
#ifdef TUNER
		else if (token == "tune")
		{
			auto epd_file = is >> token ? token : "quiet.epd";
			auto threads = is >> token ? token : "1";

			tuner::tune(epd_file, stoi(threads));
		}
#endif
		else
		{
		}
	} while (token != "quit" && argc == 1);
	// if loop is broken with 'quit', exit and destroy thread pool
	thread_pool.exit();
}

// read input stream and parse for meaningful UCI options
void set_option(std::istringstream& input)
{
	std::string token;
	input >> token;

	if (token == "name")
	{
		while (input >> token)
		{
			if (token == "Hash")
			{
				input >> token;
				input >> token;
				uci_hash = stoi(token);
				main_hash.init(uci_hash);
				acout() << "info string Hash " << uci_hash << " MB" << std::endl;
				break;
			}
			if (token == "Threads")
			{
				input >> token;
				input >> token;
				uci_threads = stoi(token);
				thread_pool.change_thread_count(uci_threads);
				if (uci_threads == 1)
					acout() << "info string Threads " << uci_threads << " thread" << std::endl;
				else
					acout() << "info string Threads " << uci_threads << " threads" << std::endl;
				break;
			}
			if (token == "MultiPV")
			{
				input >> token;
				input >> token;
				uci_multipv = stoi(token);
				acout() << "info string MultiPV " << uci_multipv << std::endl;
				break;
			}
			if (token == "Contempt")
			{
				input >> token;
				input >> token;
				uci_contempt = stoi(token);
				acout() << "info string Contempt " << uci_contempt << std::endl;
				break;
			}
			if (token == "SyzygyProbeDepth")
			{
				input >> token;
				input >> token;
				uci_syzygy_probe_depth = stoi(token);
				acout() << "info string SyzygyProbeDepth " << uci_syzygy_probe_depth << std::endl;
				break;
			}
			if (token == "SyzygyProbeLimit")
			{
				input >> token;
				input >> token;
				uci_syzygy_probe_limit = stoi(token);
				acout() << "info string SyzygyProbeLimit " << uci_syzygy_probe_limit << std::endl;
				break;
			}
			if (token == "SearchType")
			{
				input >> token;
				input >> token;
				uci_search = token;
				acout() << "info string SearchType " << uci_search << std::endl;
				break;
			}
			
			if (token == "Ponder")
			{
				input >> token;
				input >> token;
				if (token == "true")
					uci_ponder = true;
				else
					uci_ponder = false;
				acout() << "info string Ponder " << uci_ponder << std::endl;
				break;
			}
			if (token == "UCI_Chess960")
			{
				input >> token;
				input >> token;
				if (token == "true")
					uci_chess960 = true;
				else
					uci_chess960 = false;
				acout() << "info string UCI_Chess960 " << uci_chess960 << std::endl;
				break;
			}
			if (token == "ClearHash")
			{
				main_hash.clear();
				acout() << "info string Hash: cleared" << std::endl;
				break;
			}
			if (token == "Syzygy50MoveRule")
			{
				input >> token;
				input >> token;
				if (token == "true")
					uci_syzygy_50_move_rule = true;
				else
					uci_syzygy_50_move_rule = false;
				acout() << "info string Syzygy50MoveRule " << uci_syzygy_50_move_rule << std::endl;
				break;
			}
			if (token == "SyzygyPath")
			{
				input >> token;
				input >> token;
				uci_syzygy_path = token;
				egtb::syzygy_init(uci_syzygy_path);
				acout() << "info string SyzygyPath " << uci_syzygy_path << std::endl;
				break;
			}			
		}
	}
}

// read input stream for 'go' parameters
// including time and inc, etc.
void go(position& pos, std::istringstream& is)
{
	search_param param;
	std::string token;
	param.infinite = 1;

	while (is >> token)
	{
		if (token == "wtime")
		{
			is >> param.time[white];
			param.infinite = 0;
		}
		else if (token == "btime")
		{
			is >> param.time[black];
			param.infinite = 0;
		}
		else if (token == "winc")
		{
			is >> param.inc[white];
			param.infinite = 0;
		}
		else if (token == "binc")
		{
			is >> param.inc[black];
			param.infinite = 0;
		}
		else if (token == "movestogo")
		{
			is >> param.moves_to_go;
			param.infinite = 0;
		}
		else if (token == "depth")
		{
			is >> param.depth;
			param.infinite = 0;
		}
		else if (token == "infinite")
			param.infinite = 1;
	}
	if (uci_search == "random")
		random(pos);
	else			
		thread_pool.begin_search(pos, param);
}

// convert fen to internal position representation
void set_position(position& pos, std::istringstream& is)
{
	auto move = no_move;
	std::string token, fen;

	is >> token;

	if (token == "startpos")
	{
		fen = startpos;
		is >> token;
	}
	else if (token == "fen")
		while (is >> token && token != "moves")
			fen += token + " ";
	else
		return;

	pos.set(fen, false, thread_pool.main());

	while (is >> token && (move = util::move_from_string(pos, token)) != no_move)
	{
		pos.play_move(move);
		pos.increase_game_ply();
	}
}

std::string trim(const std::string& str, const std::string& whitespace)
{
	const auto str_begin = str.find_first_not_of(whitespace);
	if (str_begin == std::string::npos)
		return "";

	const auto str_end = str.find_last_not_of(whitespace);
	const auto str_range = str_end - str_begin + 1;

	return str.substr(str_begin, str_range);
}

std::string sq(const square sq)
{
	return std::string
	{
		static_cast<char>('a' + file_of(sq)), static_cast<char>('1' + rank_of(sq))
	};
}


