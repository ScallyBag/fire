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

#ifdef _WIN32
#include <Windows.h>
#else
#include <unistd.h>
#endif

#include <sstream>

#include "../define.h"
#include "../fire.h"
#include "../macro/file.h"
#include "../position.h"
#include "../movegen.h"
#include "../macro/rank.h"
#include "../util/util.h"

namespace util
{
	// return string with 'program' 'version' 'platform' and 'instruction set'
	// see fire.h
	std::string engine_info()
	{
		std::stringstream ss;
		ss << program << " " << version << " ";
		ss << platform << " " << bmis;
		ss << std::endl;
		return ss.str();
	}
	
	// return string with 'author'
	std::string engine_author()
	{
		std::stringstream ss;
		ss << author << std::endl;
		return ss.str();
	}

	// return # of cores
	std::string core_info()
	{
		std::stringstream ss;

#ifdef _WIN32
		// if windows
		SYSTEM_INFO sys_info;
		GetSystemInfo(&sys_info);
		ss << "info string " << sys_info.dwNumberOfProcessors << " available cores" << std::endl;
#else
		// if linux
		ss << "info string " << sysconf(_SC_NPROCESSORS_ONLN) << " available cores" << std::endl;
#endif

		return ss.str();
	}

	// convert from internal move format to ascii
	std::string move_to_string(const uint32_t move, const position& pos)
	{
		char s_move[6]{};

		const auto from = from_square(move);
		auto to = to_square(move);

		if (move == no_move || move == null_move)
			return "";

		if (move_type(move) == castle_move && pos.is_chess960())
			to = pos.castle_rook_square(to);

		s_move[0] = 'a' + static_cast<char>(file_of(from));
		s_move[1] = '1' + static_cast<char>(rank_of(from));
		s_move[2] = 'a' + static_cast<char>(file_of(to));
		s_move[3] = '1' + static_cast<char>(rank_of(to));

		if (move < static_cast<uint32_t>(promotion_p))
			return std::string(s_move, 4);

		s_move[4] = "   nbrq"[promotion_piece(move)];
		return std::string(s_move, 5);
	}

	// convert from ascii string to internal move format
	uint32_t move_from_string(const position& pos, std::string& str)
	{
		if (pos.is_chess960())
		{
			if (str == "O-O")
				str = move_to_string(make_move(castle_move, pos.king(pos.on_move()), relative_square(pos.on_move(), g1)), pos);
			else if (str == "O-O-O")
				str = move_to_string(make_move(castle_move, pos.king(pos.on_move()), relative_square(pos.on_move(), c1)), pos);
		}

		if (str.length() == 5)
			str[4] = static_cast<char>(tolower(str[4]));

		for (const auto& new_move : s_legal_move_list(pos))
			if (str == move_to_string(new_move, pos))
				return new_move;

		return no_move;
	}
}

// display ascii representation of position (for use in bench and perft)
std::ostream& operator<<(std::ostream& os, const position& pos)
{
	char p_chars[] =
	{
	'K','P','N','B','R','Q',
	'k','p','n','b','r','q',
	};

	auto found = false;

	for (auto r = rank_8; r >= rank_1; --r)
	{
		for (auto f = file_a; f <= file_h; ++f)
		{
			const auto pc = util::piece_to_char[pos.piece_on_square(make_square(f, r))];
			for (auto i = 0; i <= 11; ++i)
			{
				if (pc == p_chars[i])
				{
					found = true;
					break;
				}
			}
			if (found)
				os << " " << pc;
			else
				os << " " << ".";
			found = false;
		}
		os << "\n";
	}
	return os;
}