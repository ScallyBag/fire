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
	bool has_two_mobile_pieces(const position& pos);
	bool two_mobile_pieces(const position& pos);
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

	inline auto space_threshold = 592; // 2 * mat_queen + 4 * mat_rook + 2 * mat_knight // sf 12222 (* 100 / 256 = 4775) 

#ifdef TUNER
	// init mobility mult
	inline auto mob_factor_p = 270;
	inline auto mmrq_factor_p = 0;
	inline auto mmfq_factor_p = 6;
	inline auto mmc_factor_p = 2;
	inline auto mmr_factor_p = 0;
	inline auto mme_factor_p = 5;

	inline auto mob_factor_b1 = 256;
	inline auto mob_factor_b2 = 249;
	inline auto mmrq_factor_b2 = 8;
	inline auto mmfq_factor_b2 = 3;
	inline auto mmc_factor_b2 = 0;
	inline auto mmr_factor_b2 = 3;
	inline auto mme_factor_b2 = 4;

	inline auto mob_factor_r = 255;
	inline auto mmrq_factor_r = 1;
	inline auto mmfq_factor_r = 5;
	inline auto mmc_factor_r = 6;
	inline auto mmr_factor_r = 1;
	inline auto mme_factor_r = 2;

	inline auto mob_factor_q = 272;
	inline auto mmrq_factor_q = 2;
	inline auto mmfq_factor_q = 4;
	inline auto mmc_factor_q = 1;
	inline auto mmr_factor_q = 2;
	inline auto mme_factor_q = 8;

	// init pawn mobility values
	inline auto pawn_mg_mult = 207.32;
	inline auto pawn_mg_sub = 417.0;
	inline auto pawn_eg_mult = 252.68;
	inline auto pawn_eg_sub = 509.0;

	// init bishop mobility values
	inline auto b1_mg_mult = 92.43;
	inline auto b1_mg_sub = 171.0;
	inline auto b1_eg_mult = 104.75;
	inline auto b1_eg_sub = 194.0;

	inline auto b2_mg_mult = 221.48;
	inline auto b2_mg_sub = 374.0;
	inline auto b2_eg_mult = 203.99;
	inline auto b2_eg_sub = 344.0;

	// init rook mobility values
	inline auto rook_mg_mult = 125.90;
	inline auto rook_mg_sub = 190.0;
	inline auto rook_eg_mult = 218.96;
	inline auto rook_eg_sub = 331.0;
	inline auto mob_r_mult = 7;
	inline auto mob_r_div = 8;

	// init queen mobility values
	inline auto queen_mg_mult = 203.42;
	inline auto queen_mg_sub = 616.0;
	inline auto queen_eg_mult = 165.33;
	inline auto queen_eg_sub = 555.0;

	// init distance values
	inline auto p_k_distance = 1114123;
	inline auto p_k_distance_mult = 3;
	inline auto b_k_distance = 65545;
	inline auto b_k_distance_mult = 3;

	// init pawn-bishop color values
	inline auto pawn_on_bishop_color = -1769515;
	inline auto pawn_on_other_bishop_color = 3014673;
	inline auto pawn_file_width_mg = 0;
	inline auto pawn_file_width_eg = 2;
	inline auto threats_score = 14418116;

	// init passed pawn values
	inline int pp_dvd_mgfactor = 46;
	inline int pp_dvd_egfactor = 33;
	inline int pp_ndvd_mgfactor = 49;
	inline int pp_ndvd_egfactor = 34;

	inline auto pp_fp_base_mg = 1;
	inline auto pp_fp_base_eg = 3;
	inline auto pp_fp_mg = 10;
	inline auto pp_fp_eg = 60;
	inline auto pp_fp_mul = 272;
	inline auto pp_fp_div = 256;

	inline auto pp_as_base_mg = 1;
	inline auto pp_as_base_eg = 3;
	inline auto pp_as_mg = 10;
	inline auto pp_as_eg = 36;
	inline auto pp_as_mul = 304;
	inline auto pp_as_div = 256;

	inline auto pp_ab_base_mg = 1;
	inline auto pp_ab_base_eg = 3;

	inline int pp_support_proximity_factor = 30;
	inline int pp_mk_kdfp_factor = 40;
	inline int pp_yk_kdfp_factor = 76;
	inline int pp_mk_factor = 32;
	inline int pp_mk_div = 35;
	inline int pp_yk_factor = 32;
	inline int pp_yk_div = 35;

	// calculate_scale_factor
	inline auto sf_mult = 3;
	inline auto sf_div = 4;

	// eval_bishops
	inline auto bishop_in_front_of_king = 7602176;
	inline auto bishop_in_corner = 6553753;
	inline auto trapped_bishop_extra = 69731368;
	inline auto trapped_bishop = 33030648;
	inline auto bishop_dominates_pawn = 2097182;
	inline auto k_zone_attack_bonus = 8;

	// eval_initiative	
	inline auto initiative_mult = 38;

	// eval_king_attack
	inline auto k_attack_index_factor = 16;
	inline auto k_attack_pin_factor = 12;
	inline auto k_attack_sd_factor = 11;
	inline auto cspan_safe = 70;
	inline auto cspan = 30;
	inline auto csbab_safe = 54;
	inline auto csbab = 22;
	inline auto csrar_safe = 70;
	inline auto csrar = 30;
	inline auto qcayk_all = 86;
	inline auto qcayk = 38;
	inline auto queen_check_bonus = 120;

	// eval_knights
	inline auto knight_attack_king = 24;
	inline auto p_mobility_add = 16;
	inline auto p_mobility_div = 32;

	// eval_passed_pawns
	inline auto passed_pawn_mk_mult = 3;
	inline auto passed_pawn_mk_div = 4;
	inline auto passed_pawn_yk_mult = 3;
	inline auto passed_pawn_yk_div = 4;
	inline auto passed_pawn_mk_md_mul = 2;
	inline auto passed_pawn_mk_md_div = 4;
	inline auto passed_pawn_yk_md_mul = 2;
	inline auto passed_pawn_yk_md_div = 4;
	inline auto bb_behind_passed_pawn_bonus = 6488502;

	// eval_queens
	inline auto queen_attack_king = 24;
	inline auto queen_attack_king_zone = 8;
	inline auto q_mobility_add = 32;
	inline auto q_mobility_div = 64;

	// eval_rooks
	inline auto uncastled_penalty = 29556897;
	inline auto rook_attacks_king = 8;
	inline auto rook_traps_king_on_7th = 6684932;
	inline auto no_pawn = 19398907;
	inline auto pawn_attacks = 2097222;
	inline auto pawn_no_attack = 11731094;
	inline auto r_mobility_add = 16;
	inline auto r_mobility_div = 32;

	// eval_space
	inline auto space_weight_mult = 3;
	inline auto space_weight_div = 16;

	// eval_strong_squares
	inline auto safety_for_pawn_rbp = 3670059;
	inline auto strong_p_in_front_of_pawn = 1441846;
	inline auto strong_square_pb = 6488176;
	inline auto strong_square_pb_extra = 16318582;
	inline auto pb_behind_pawn = 3342348;
	inline auto protected_piece = 5767214;

	// eval_threats
	inline auto hanging_pawn_threat = 26083619;
	inline auto hanging_pieces = 17498230;
	inline auto king_threat_single = 2490697;
	inline auto king_threat_multiple = 6488796;
	inline auto pawn_advance = 11272272;

	// eval
	inline auto blocked_pawns_mg = 43;
	inline auto blocked_pawns_eg = 167;
	inline auto mg_mgvalue_mult = 106;
	inline auto mg_egvalue_mult = 6;
	inline auto eg_mgvalue_mult = 13;
	inline auto eg_egvalue_mult = 87;
	inline auto eval_mult = 35;
	inline auto conversion_mult = 115;
	inline auto conversion_div = 128;
	inline auto eval_div = 32;
	inline auto eval_value_div = 8;
	inline auto flank_double_attack = 1835008;
	inline auto pawn_contempt_mult = 2;
	inline auto knight_contempt_mult = 2;
	inline auto bishop_contempt_mult = 3;
	inline auto rook_contempt_mult = 4;
	inline auto queen_contempt_mult = 8;
	inline auto contempt_mult = 4;
#endif // TUNER

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
