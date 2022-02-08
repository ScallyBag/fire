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

#pragma once
#include "fire.h"
#include "position.h"
#include "macro/square.h"

namespace pawn
{
	inline score king_pawn_distance[17][8];
	inline int chain_score[2][2][3][num_ranks];

	inline int pawn_shield[8];
	inline int pawn_storm[8];
	inline int storm_half_open_file[8];
	inline int attack_on_file[8];

	const auto distance = 6;
	const auto distance_2_div = 2;
	const auto distance_3_div = 3;
	const auto distance_4_div = 4;
	const auto distance_5_div = 5;

	constexpr score pps(const int mg, const int eg)
	{
		return make_score(mg, eg);
	}

	constexpr int ev(const int x)
	{
		return x * 2;
	}

#ifdef TUNER
	// 	eval_pawns
	inline auto center_bind = 4259831;
	inline auto multiple_passed_pawns = 3408076;
	inline auto second_row_fixed = 1114131;

	// init
	inline auto max_safety_bonus = ev(258);
	inline auto pawn_unsupported = 5505051;
	inline auto chain_mult = 3;
	inline auto chain_div = 2;
	inline auto file_factor_mult = 64;

	// eval_shelter_storm
	inline auto ss_base = 100;
	inline auto ss_safety_factor = 3;
	inline auto ss_danger_factor = 3;

	// calculate_king_safety
	inline auto safe_bonus_div = 220;
	inline auto safe_bonus_mult = 8;
	inline auto safe_bonus_mult_r34 = 16;
	inline auto safe_bonus_mult_r5 = 8;
	inline auto king_1st_rank = static_cast<int>(-6553876);
	inline auto king_near_enemy_pawns = static_cast<int>(43);
#endif
	//ps factors
	inline auto mg_mg_mult = 15399;
	inline auto mg_eg_mult = 852;
	inline auto eg_mg_mult = 2301;
	inline auto eg_eg_mult = 15052;
	inline auto ps_div = 5952;

	//pawn shield/storm
	inline int shield_factor[3] =
	{
		74, 69, 64
	};

	inline int storm_factor[3] =
	{
		64, 48, 64
	};

	inline int pawn_shield_constants[8] =
	{
		0, 27, 22, 10, 5, 0, 0, 0
	};

	inline int pawn_storm_constants[8] =
	{
		-2, 0, 0, -6, -13, -40, 0, 0
	};

	//pawn phalanx
	inline int phalanx_seed[num_ranks] =
	{
		0, 10, 13, 33, 66, 105, 196, 0
	};

	inline int seed[num_ranks] =
	{
		0, 0, 15, 10, 57, 75, 135, 0
	};

	inline int remaining_score[2] =
	{
		18743424, 13631553
	};

	inline int un_supported_pawn[2] =
	{
		8257574, 5505051
	};

	inline int pawn_attacker_score[num_ranks] =
	{
		0, 0, 0, 0, 5636163, 11403400, 0, 0
	};

	// passed pawn
	inline int passed_pawn_values[8] =
	{
		0, 720913, 720913, 3211339, 8454331, 16253284, 26608198, 0
	};

	inline int passed_pawn_values_2[8] =
	{
		0, 1441828, 1441828, 6553750, 16974200, 32572105, 53347468, 0
	};

	// doubled/isolated pawn
	inline score doubled_pawn[4] =
	{
		static_cast<score>(4063396), static_cast<score>(6029492), static_cast<score>(6881459), static_cast<score>(6881459)
	};

	inline int isolated_pawn[2][num_files] =
	{
		{
			10682526, 15401136, 16973996, 16973996, 16973996, 16973996, 15401136, 10682526
		},
		{
			7077993, 10223734, 11272308, 11272308, 11272308, 11272308, 10223734, 7077993
		}
	};

	// pawn tables
	inline int shelter_weakness[][num_ranks] =
	{
		{96, 20, 26, 50, 86, 88, 98},
		{120, 0, 28, 76, 88, 102, 104},
		{100, 6, 54, 78, 76, 92, 100},
		{80, 10, 44, 68, 86, 90, 118}
	};

	inline int storm_danger[][4][num_ranks] =
	{
		{
		{0, 66, 134, 38, 32},
		{0, 56, 138, 36, 22},
		{0, 42, 114, 42, 26},
		{0, 68, 124, 56, 32}
		},
		{
		{20, 42, 100, 56, 20},
		{22, 20, 98, 40, 14},
		{22, 38, 102, 36, 18},
		{28, 18, 108, 42, 26}
		},
		{
		{0, 0, 74, 14, 2},
		{0, 0, 150, 30, 4},
		{0, 0, 160, 22, 4},
		{0, 0, 166, 24, 12}
		},
		{
		{0, -282, -280, 56, 30},
		{0, 58, 140, 38, 18},
		{0, 64, 142, 48, 32},
		{0, 60, 126, 50, 18}
		}
	};

	inline score ps(const int mg, const int eg)
	{
		return make_score((mg * mg_mg_mult + eg * mg_eg_mult) / ps_div, (mg * eg_mg_mult + eg * eg_eg_mult) / ps_div);
	}

	const uint64_t attack_files[num_files] =
	{
		file_a_bb | file_b_bb | file_c_bb,
		file_a_bb | file_b_bb | file_c_bb,
		file_b_bb | file_c_bb | file_d_bb,
		file_c_bb | file_d_bb | file_e_bb,
		file_d_bb | file_e_bb | file_f_bb,
		file_e_bb | file_f_bb | file_g_bb,
		file_f_bb | file_g_bb | file_h_bb,
		file_f_bb | file_g_bb | file_h_bb
	};

	inline uint64_t file_behind(uint64_t x)
	{
		x = x | x >> 8;
		x = x | x >> 16;
		x = x | x >> 32;
		return x;
	}

	inline uint64_t file_front(uint64_t x)
	{
		x = x | x << 8;
		x = x | x << 16;
		x = x | x << 32;
		return x;
	}

	inline uint64_t file_front_rear(uint64_t x)
	{
		x = x | x >> 8 | x << 8;
		x = x | x >> 16 | x << 16;
		x = x | x >> 32 | x << 32;
		return x;
	}

	const int doubled_pawn_distance[num_files][distance] =
	{
		{
			0, doubled_pawn[0], doubled_pawn[0] / distance_2_div,
			doubled_pawn[0] / distance_3_div, doubled_pawn[0] / distance_4_div, doubled_pawn[0] / distance_5_div
		},
		{
			0, doubled_pawn[1], doubled_pawn[1] / distance_2_div,
			doubled_pawn[1] / distance_3_div, doubled_pawn[1] / distance_4_div, doubled_pawn[1] / distance_5_div
		},
		{
			0, doubled_pawn[2], doubled_pawn[2] / distance_2_div,
			doubled_pawn[2] / distance_3_div, doubled_pawn[2] / distance_4_div, doubled_pawn[2] / distance_5_div
		},
		{
			0, doubled_pawn[3], doubled_pawn[3] / distance_2_div,
			doubled_pawn[3] / distance_3_div, doubled_pawn[3] / distance_4_div, doubled_pawn[3] / distance_5_div
		},
		{
			0, doubled_pawn[3], doubled_pawn[3] / distance_2_div,
			doubled_pawn[3] / distance_3_div, doubled_pawn[3] / distance_4_div, doubled_pawn[3] / distance_5_div
		},
		{
			0, doubled_pawn[2], doubled_pawn[2] / distance_2_div,
			doubled_pawn[2] / distance_3_div, doubled_pawn[2] / distance_4_div, doubled_pawn[2] / distance_5_div
		},
		{
			0, doubled_pawn[1], doubled_pawn[1] / distance_2_div,
			doubled_pawn[1] / distance_3_div, doubled_pawn[1] / distance_4_div, doubled_pawn[1] / 5
		},
		{
			0, doubled_pawn[0], doubled_pawn[0] / distance_2_div,
			doubled_pawn[0] / distance_3_div, doubled_pawn[0] / distance_4_div, doubled_pawn[0] / 5
		}
	};

	struct pawn_hash_entry
	{
		[[nodiscard]] int pawns_score() const
		{
			return pscore;
		}

		[[nodiscard]] uint64_t pawn_attack(const side color) const
		{
			return p_attack[color];
		}

		[[nodiscard]] uint64_t passed_pawns(const side color) const
		{
			return passed_p[color];
		}

		[[nodiscard]] uint64_t safe_for_pawn(const side color) const
		{
			return safe_pawn[color];
		}

		[[nodiscard]] int pawn_range(const side color) const
		{
			return pawn_span[color];
		}

		[[nodiscard]] int semi_open_side(const side color, const file f, const bool left_side) const
		{
			return half_open_lines[color] & (left_side ? (1 << f) - 1 : ~((1 << (f + 1)) - 1));
		}

		[[nodiscard]] int pawns_on_color(const side color, const square sq) const
		{
			return pawns_sq_color[color][!!(dark_squares & sq)];
		}

		[[nodiscard]] int pawns_not_on_color(const side color, const square sq) const
		{
			return pawns_sq_color[color][!(dark_squares & sq)];
		}

		template <side me>
		score king_safety(const position& pos)
		{
			if (king_square[me] != pos.king(me) || castle_possibilities[me] != pos.castling_possible(me))
				my_king_safety[me] = calculate_king_safety<me>(pos);
			return my_king_safety[me];
		}

		template <side me>
		score calculate_king_safety(const position& pos);

		uint64_t key;
		uint64_t passed_p[num_sides];
		uint64_t p_attack[num_sides];
		uint64_t safe_pawn[num_sides];
		int pscore;
		score my_king_safety[num_sides];
		uint8_t king_square[num_sides];
		uint8_t castle_possibilities[num_sides];
		uint8_t half_open_lines[num_sides];
		uint8_t pawn_span[num_sides];
		int asymmetry;
		uint8_t pawns_sq_color[num_sides][num_sides];
		int average_line, n_pawns;
		bool conversion_difficult;
		int safety[num_sides];
		int file_width;
		char padding[20];
	};

	static_assert(offsetof(struct pawn_hash_entry, half_open_lines) == 72, "offset wrong");
	static_assert(sizeof(pawn_hash_entry) == 128, "Pawn Hash Entry size incorrect");

	template <class entry, int size>
	struct pawn_hash_table
	{
		entry* operator[](const uint64_t key)
		{
			static_assert(sizeof(entry) == 32 || sizeof(entry) == 128, "Wrong size");
			return reinterpret_cast<entry*>(reinterpret_cast<char*>(pawn_hash_mem_) + (static_cast<uint32_t>(key) & (size - 1) * sizeof(entry)));
		}

	private:
		CACHE_ALIGN entry pawn_hash_mem_[size];
	};

	void init();
	pawn_hash_entry* probe(const position& pos);
	const int pawn_hash_size = 16384;
	typedef pawn_hash_table<pawn_hash_entry, pawn_hash_size> pawn_hash;
}

	inline square square_in_front(const side color, const square sq)
{
	return color == white ? sq + north : sq + south;
}

inline square square_behind(const side color, const square sq)
{
	return color == white ? sq - north : sq - south;
}
