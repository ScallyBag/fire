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

#include "bitboard.h"
#include "fire.h"
#include "position.h"

class position;

namespace evaluate
{
	void init();

	int eval(const position& pos, int alpha, int beta);
	int eval_after_null_move(int eval);
	extern score safety_table[1024];

	constexpr score es(const int mg, const int eg)
	{
		return make_score(mg, eg);
	}

	// declare evaluation arrays
	inline score safety_table[1024];

	inline uint32_t mob_mult_p[64];
	inline uint32_t mob_mult_b1[64];
	inline uint32_t mob_mult_b2[64];
	inline uint32_t mob_mult_r[64];
	inline uint32_t mob_mult_q[64];

	inline score mobility_p[256];
	inline score mobility_b1[256];
	inline score mobility_b2[256];
	inline score mobility_r[256];
	inline score mobility_q[256];

	inline score distance_p_k[8];
	inline score distance_b_k[8];
	inline score pawn_on_color_bishop[9];
	inline score pawn_other_color_bishop[9];
	inline score pawn_file_width[9];

	inline int passed_pawn_free_passage[8];
	inline int passed_pawn_advance_supported[8];
	inline score passed_pawn_advance_blocked[8];
	inline score passed_pawn_not_dvd[8];
	inline score passed_pawn_dvd[8];
	inline score passed_pawn_my_k[6][8];
	inline score passed_pawn_your_k[6][8];
	inline score threats[9];

	//eval bonuses/penalties

	inline int passed_pawn_proximity[8] =
	{
		13, 9, 6, 4, 3, 2, 1, 0
	};

	// threat arrays
	inline int piece_threat[num_piecetypes - 1] =
	{
		0, 0, 721072, 16842952, 17301724, 27853324, 19661400
	};

	inline int rook_threat[num_piecetypes - 1] =
	{
		0, 0, 524420, 15466801, 15466785, 721076, 13369577
	};

	inline int pawn_threat[num_piecetypes - 1] =
	{
		0, 0, 0, 65274484, 49152593, 81789952, 76743673
	};

	//bishop pin array
	inline int bishop_pin[num_sides][num_pieces] =
	{
		{
		0, 0, 0, 9437328, 0, 6291552, 6291552, 0,
		0, 0, 0, 12583104, 0, 23593368, 23593368, 0
		},
		{
		0, 0, 0, 12583104, 0, 23593368, 23593368, 0,
		0, 0, 0, 9437328, 0, 6291552, 6291552, 0
		}
	};

	// mobility tables
	inline int mob_mult_const[64] =
	{
		1, 1, 1, 1, 1, 1, 1, 1,
		1, 1, 1, 1, 1, 1, 1, 1,
		1, 1, 1, 1, 1, 1, 1, 1,
		1, 1, 1, 1, 1, 1, 1, 1,
		1, 1, 1, 1, 1, 1, 1, 1,
		1, 1, 1, 1, 1, 1, 1, 1,
		1, 1, 1, 1, 1, 1, 1, 1,
		1, 1, 1, 1, 1, 1, 1, 1
	};

	inline int mob_mult_rank_quad[64] =
	{
		-9, -9, -9, -9, -9, -9, -9, -9,
		-3, -3, -3, -3, -3, -3, -3, -3,
		1, 1, 1, 1, 1, 1, 1, 1,
		3, 3, 3, 3, 3, 3, 3, 3,
		3, 3, 3, 3, 3, 3, 3, 3,
		1, 1, 1, 1, 1, 1, 1, 1,
		-3, -3, -3, -3, -3, -3, -3, -3,
		-9, -9, -9, -9, -9, -9, -9, -9
	};

	inline int mob_mult_file_quad[64] =
	{
		-9, -3, 1, 3, 3, 1, -3, -9,
		-9, -3, 1, 3, 3, 1, -3, -9,
		-9, -3, 1, 3, 3, 1, -3, -9,
		-9, -3, 1, 3, 3, 1, -3, -9,
		-9, -3, 1, 3, 3, 1, -3, -9,
		-9, -3, 1, 3, 3, 1, -3, -9,
		-9, -3, 1, 3, 3, 1, -3, -9,
		-9, -3, 1, 3, 3, 1, -3, -9
	};

	inline int mob_mult_center[64] =
	{
		4, 3, 2, 1, 1, 2, 3, 4,
		3, 2, 1, 0, 0, 1, 2, 3,
		2, 1, 0, -1, -1, 0, 1, 2,
		1, 0, -1, -2, -2, -1, 0, 1,
		1, 0, -1, -2, -2, -1, 0, 1,
		2, 1, 0, -1, -1, 0, 1, 2,
		3, 2, 1, 0, 0, 1, 2, 3,
		4, 3, 2, 1, 1, 2, 3, 4
	};

	inline int mob_mult_rank[64] =
	{
		-3, -3, -3, -3, -3, -3, -3, -3,
		-2, -2, -2, -2, -2, -2, -2, -2,
		-1, -1, -1, -1, -1, -1, -1, -1,
		0, 0, 0, 0, 0, 0, 0, 0,
		1, 1, 1, 1, 1, 1, 1, 1,
		2, 2, 2, 2, 2, 2, 2, 2,
		3, 3, 3, 3, 3, 3, 3, 3,
		4, 4, 4, 4, 4, 4, 4, 4
	};

	inline int mob_mult_edge[64] =
	{
		-3, -3, -3, -3, -3, -3, -3, -3,
		-3, -1, -1, -1, -1, -1, -1, -3,
		-3, -1, 1, 1, 1, 1, -1, -3,
		-3, -1, 1, 3, 3, 1, -1, -3,
		-3, -1, 1, 3, 3, 1, -1, -3,
		-3, -1, 1, 1, 1, 1, -1, -3,
		-3, -1, -1, -1, -1, -1, -1, -3,
		-3, -3, -3, -3, -3, -3, -3, -3
	};

	// king danger table
	inline int king_danger[256] =
	{
		0, 6, 19, 39, 71, 110, 162, 221, 286, 357, 442, 526, 624, 728, 838, 955,
		1079, 1202, 1332, 1475, 1612, 1755, 1904, 2060, 2210, 2366, 2522, 2684, 2847, 3009, 3165, 3328,
		3490, 3653, 3815, 3971, 4134, 4290, 4446, 4602, 4751, 4901, 5050, 5193, 5336, 5473, 5609, 5746,
		5876, 6006, 6129, 6253, 6370, 6487, 6604, 6714, 6818, 6922, 7026, 7124, 7221, 7312, 7403, 7488,
		7572, 7657, 7735, 7806, 7884, 7956, 8021, 8092, 8151, 8216, 8274, 8333, 8391, 8443, 8495, 8541,
		8593, 8638, 8684, 8723, 8768, 8807, 8840, 8879, 8911, 8950, 8983, 9015, 9041, 9074, 9100, 9126,
		9152, 9178, 9197, 9223, 9243, 9262, 9282, 9301, 9321, 9340, 9353, 9373, 9386, 9405, 9418, 9431,
		9444, 9457, 9470, 9483, 9490, 9503, 9516, 9522, 9535, 9542, 9548, 9561, 9568, 9574, 9581, 9587,
		9594, 9594, 9594, 9594, 9594, 9594, 9594, 9594, 9594, 9594, 9594, 9594, 9594, 9594, 9594, 9594,
		9594, 9594, 9594, 9594, 9594, 9594, 9594, 9594, 9594, 9594, 9594, 9594, 9594, 9594, 9594, 9594,
		9594, 9594, 9594, 9594, 9594, 9594, 9594, 9594, 9594, 9594, 9594, 9594, 9594, 9594, 9594, 9594,
		9594, 9594, 9594, 9594, 9594, 9594, 9594, 9594, 9594, 9594, 9594, 9594, 9594, 9594, 9594, 9594,
		9594, 9594, 9594, 9594, 9594, 9594, 9594, 9594, 9594, 9594, 9594, 9594, 9594, 9594, 9594, 9594,
		9594, 9594, 9594, 9594, 9594, 9594, 9594, 9594, 9594, 9594, 9594, 9594, 9594, 9594, 9594, 9594,
		9594, 9594, 9594, 9594, 9594, 9594, 9594, 9594, 9594, 9594, 9594, 9594, 9594, 9594, 9594, 9594,
		9594, 9594, 9594, 9594, 9594, 9594, 9594, 9594, 9594, 9594, 9594, 9594, 9594, 9594, 9594, 9594
	};

	// king flank attack table
	inline uint64_t bb_king_flank_attack[num_sides][num_files] =
	{
		{
			(rank_5_bb | rank_4_bb | rank_3_bb | rank_2_bb | rank_1_bb) & (file_a_bb | file_b_bb | file_c_bb | file_d_bb),
			(rank_5_bb | rank_4_bb | rank_3_bb | rank_2_bb | rank_1_bb) & (file_a_bb | file_b_bb | file_c_bb | file_d_bb),
			(rank_5_bb | rank_4_bb | rank_3_bb | rank_2_bb | rank_1_bb) & (file_a_bb | file_b_bb | file_c_bb | file_d_bb),
			(rank_5_bb | rank_4_bb | rank_3_bb | rank_2_bb | rank_1_bb) & (file_c_bb | file_d_bb | file_e_bb | file_f_bb),
			(rank_5_bb | rank_4_bb | rank_3_bb | rank_2_bb | rank_1_bb) & (file_c_bb | file_d_bb | file_e_bb | file_f_bb),
			(rank_5_bb | rank_4_bb | rank_3_bb | rank_2_bb | rank_1_bb) & (file_e_bb | file_f_bb | file_g_bb | file_h_bb),
			(rank_5_bb | rank_4_bb | rank_3_bb | rank_2_bb | rank_1_bb) & (file_e_bb | file_f_bb | file_g_bb | file_h_bb),
			(rank_5_bb | rank_4_bb | rank_3_bb | rank_2_bb | rank_1_bb) & (file_e_bb | file_f_bb | file_g_bb | file_h_bb)
		},
		{
			(rank_4_bb | rank_5_bb | rank_6_bb | rank_7_bb | rank_8_bb) & (file_a_bb | file_b_bb | file_c_bb | file_d_bb),
			(rank_4_bb | rank_5_bb | rank_6_bb | rank_7_bb | rank_8_bb) & (file_a_bb | file_b_bb | file_c_bb | file_d_bb),
			(rank_4_bb | rank_5_bb | rank_6_bb | rank_7_bb | rank_8_bb) & (file_a_bb | file_b_bb | file_c_bb | file_d_bb),
			(rank_4_bb | rank_5_bb | rank_6_bb | rank_7_bb | rank_8_bb) & (file_c_bb | file_d_bb | file_e_bb | file_f_bb),
			(rank_4_bb | rank_5_bb | rank_6_bb | rank_7_bb | rank_8_bb) & (file_c_bb | file_d_bb | file_e_bb | file_f_bb),
			(rank_4_bb | rank_5_bb | rank_6_bb | rank_7_bb | rank_8_bb) & (file_e_bb | file_f_bb | file_g_bb | file_h_bb),
			(rank_4_bb | rank_5_bb | rank_6_bb | rank_7_bb | rank_8_bb) & (file_e_bb | file_f_bb | file_g_bb | file_h_bb),
			(rank_4_bb | rank_5_bb | rank_6_bb | rank_7_bb | rank_8_bb) & (file_e_bb | file_f_bb | file_g_bb | file_h_bb)
		}
	};

	// bishop tables
	const uint64_t bb_b_dominates_p[2][64] =
	{
		{
			0, 0, 0, 0, 0, 0, 0, 0,
			bb(a5), bb(b5), bb(c5), bb(d5), bb(e5), bb(f5), bb(g5), bb(h5),
			bb(a6), bb(b6), bb(c6), bb(d6), bb(e6), bb(f6), bb(g6), bb(h6),
			bb(a7), bb(b7), bb(c7), bb(d7), bb(e7), bb(f7), bb(g7), bb(h7),
			bb(a8), bb(b8), bb(c8), bb(d8), bb(e8), bb(f8), bb(g8), bb(h8),
			0, 0, 0, 0, 0, 0, 0, 0,
			0, 0, 0, 0, 0, 0, 0, 0,
			0, 0, 0, 0, 0, 0, 0, 0
		},
		{
			0, 0, 0, 0, 0, 0, 0, 0,
			0, 0, 0, 0, 0, 0, 0, 0,
			0, 0, 0, 0, 0, 0, 0, 0,
			bb(a1), bb(b1), bb(c1), bb(d1), bb(e1), bb(f1), bb(g1), bb(h1),
			bb(a2), bb(b2), bb(c2), bb(d2), bb(e2), bb(f2), bb(g2), bb(h2),
			bb(a3), bb(b3), bb(c3), bb(d3), bb(e3), bb(f3), bb(g3), bb(h3),
			bb(a4), bb(b4), bb(c4), bb(d4), bb(e4), bb(f4), bb(g4), bb(h4),
			0, 0, 0, 0, 0, 0, 0, 0
		}
	};

	const uint64_t trapped_bishop_b3_c2[2][64] =
	{
		{
			bb(b2), bb(c2), 0, 0, 0, 0, bb(f2), bb(g2),
			bb(b3), 0, 0, 0, 0, 0, 0, bb(g3),
			0, 0, 0, 0, 0, 0, 0, 0,
			0, 0, 0, 0, 0, 0, 0, 0,
			0, 0, 0, 0, 0, 0, 0, 0,
			bb(b5), 0, 0, 0, 0, 0, 0, bb(g5),
			bb(b6), 0, 0, 0, 0, 0, 0, bb(g6),
			bb(b7), bb(c7), 0, 0, 0, 0, bb(f7), bb(g7)
		},
		{
			bb(b2), bb(c2), 0, 0, 0, 0, bb(f2), bb(g2),
			bb(b3), 0, 0, 0, 0, 0, 0, bb(g3),
			bb(b4), 0, 0, 0, 0, 0, 0, bb(g4),
			0, 0, 0, 0, 0, 0, 0, 0,
			0, 0, 0, 0, 0, 0, 0, 0,
			0, 0, 0, 0, 0, 0, 0, 0,
			bb(b6), 0, 0, 0, 0, 0, 0, bb(g6),
			bb(b7), bb(c7), 0, 0, 0, 0, bb(f7), bb(g7)
		}
	};

	const uint64_t trapped_bishop_b3_c2_extra[64] =
	{
		bb(b3), bb(b3), 0, 0, 0, 0, bb(g3), bb(g3),
		bb(c2), 0, 0, 0, 0, 0, 0, bb(f2),
		bb(c3), 0, 0, 0, 0, 0, 0, bb(f3),
		0, 0, 0, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 0, 0,
		bb(c6), 0, 0, 0, 0, 0, 0, bb(f6),
		bb(c7), 0, 0, 0, 0, 0, 0, bb(f7),
		bb(b6), bb(b6), 0, 0, 0, 0, bb(g6), bb(g6)
	};

}
