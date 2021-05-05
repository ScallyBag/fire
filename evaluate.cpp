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

#include "evaluate.h"
#include "endgame.h"
#include "bitboard.h"
#include "fire.h"
#include "material.h"
#include "pawn.h"
#include "macro/score.h"
#include "thread.h"

namespace evaluate
{
	// init arrays
	void init()
		{
		// init mobility mult arrays
		for (auto n = 0; n < 64; n++)
		{
			mob_mult_p[n] = mob_factor_p * mob_mult_const[n] + ((mmrq_factor_p * mob_mult_rank_quad[n] + mmfq_factor_p * mob_mult_file_quad[n]) >> 1)
				- mmc_factor_p * mob_mult_center[n] + mmr_factor_p * mob_mult_rank[n] + mme_factor_p * mob_mult_edge[n];

			mob_mult_b1[n] = mob_factor_b1;

			mob_mult_b2[n] = mob_factor_b2 * mob_mult_const[n] + ((-mmrq_factor_b2 * mob_mult_rank_quad[n] - mmfq_factor_b2 * mob_mult_file_quad[n]) >> 1)
				+ mmc_factor_b2 * mob_mult_center[n] - mmr_factor_b2 * mob_mult_rank[n] - mme_factor_b2 * mob_mult_edge[n];

			mob_mult_r[n] = mob_factor_r * mob_mult_const[n] + ((mmrq_factor_r * mob_mult_rank_quad[n] - mmfq_factor_r * mob_mult_file_quad[n]) >> 1)
				- mmc_factor_r * mob_mult_center[n] - mmr_factor_r * mob_mult_rank[n] - mme_factor_r * mob_mult_edge[n];

			mob_mult_q[n] = mob_factor_q * mob_mult_const[n] + ((-mmrq_factor_q * mob_mult_rank_quad[n] + mmfq_factor_q * mob_mult_file_quad[n]) >> 1)
				+ mmc_factor_q * mob_mult_center[n] - mmr_factor_q * mob_mult_rank[n] - mme_factor_q * mob_mult_edge[n];
		}

		// init mobility arrays
		for (auto n = 0; n < 256; n++)
		{
			auto v38 = sqrt(0.125 * n + 1.5) - sqrt(1.5);
			mobility_p[n] = make_score(std::lround(v38 * pawn_mg_mult - pawn_mg_sub), std::lround(v38 * pawn_eg_mult - pawn_eg_sub));
			mobility_b1[n] = make_score(std::lround(v38 * b1_mg_mult - b1_mg_sub), std::lround(v38 * b1_eg_mult - b1_eg_sub));
			mobility_b2[n] = make_score(std::lround(v38 * b2_mg_mult - b2_mg_sub), std::lround(v38 * b2_eg_mult - b2_eg_sub));
			mobility_r[n] = make_score(std::lround(v38 * rook_mg_mult - rook_mg_sub) * mob_r_mult / mob_r_div, std::lround(v38 * rook_eg_mult - rook_eg_sub) * mob_r_mult / mob_r_div);
			v38 = sqrt(0.25 * n + 1.5) - sqrt(1.5);
			mobility_q[n] = make_score(std::lround(v38 * queen_mg_mult - queen_mg_sub), std::lround(v38 * queen_eg_mult - queen_eg_sub));
		}

		// init distance arrays
		for (auto n = 0; n < 8; n++)
		{
			distance_p_k[n] = static_cast<score>(p_k_distance * (p_k_distance_mult - n));
			distance_b_k[n] = static_cast<score>(b_k_distance * (b_k_distance_mult - n));
		}

		// init pawn-bishop color arrays
		for (auto n = 0; n < 9; n++)
		{
			pawn_on_color_bishop[n] = static_cast<score>(pawn_on_bishop_color * (n - 2));
			pawn_other_color_bishop[n] = static_cast<score>(pawn_on_other_bishop_color * (n - 2));
			pawn_file_width[n] = make_score(pawn_file_width_mg, pawn_file_width_eg * (n > 5 ? 9 * n - 36 : n * n - 16));
			threats[n] = static_cast<score>(threats_score * (n > 1 ? n + 2 : n));
		}

		// init passed pawn arrays
		for (auto n = 0; n <= 5; n++)
		{
			passed_pawn_dvd[n] = make_score(pp_dvd_mgfactor * (n - 1) * n, pp_dvd_egfactor * (n * n + 1));
			passed_pawn_not_dvd[n] = make_score(pp_ndvd_mgfactor * (n - 1) * n, pp_ndvd_egfactor * (n * n + 1));

			passed_pawn_free_passage[n] = make_score(pp_fp_base_mg, pp_fp_base_eg) * (n - 1) * n + mul_div(make_score(pp_fp_mg, pp_fp_eg) * (n - 1) * n, pp_fp_mul, pp_fp_div);
			passed_pawn_advance_supported[n] = make_score(pp_as_base_mg, pp_as_base_eg) * (n - 1) * n + mul_div(make_score(pp_as_mg, pp_as_eg) * (n - 1) * n, pp_as_mul, pp_as_div);
			passed_pawn_advance_blocked[n] = make_score(pp_ab_base_mg, pp_ab_base_eg) * (n - 1) * n;

			for (auto distance = 0; distance < 8; distance++)
			{
				const auto support = pp_support_proximity_factor * passed_pawn_proximity[distance];
				const auto free_path = (sqrt(distance + 1.0) - 1.0) * (static_cast<double>(n) - static_cast<double>(1)) * n;
				passed_pawn_my_k[n][distance] = make_score(0, (support - std::lround(free_path * pp_mk_kdfp_factor)) * pp_mk_factor / pp_mk_div);
				passed_pawn_your_k[n][distance] = make_score(0, (std::lround(free_path * pp_yk_kdfp_factor) - support) * pp_yk_factor / pp_yk_div);
			}
		}

		// init king_danger & safety table array
		auto prev = 0;
		for (auto n = 0; n < 128; n++)
		{
			const auto val = king_danger[n];
			safety_table[8 * n] = make_score(val, 0);
			if (n > 0)
				for (auto i = 1; i < 8; i++)
					safety_table[8 * n - 8 + i] = make_score((i * val + (8 - i) * prev) / 8, 0);
			prev = val;
		}
	}

	// calculate and return piece attack bitboard
	template <side me>
	uint64_t calculate_attack(const position& pos)
	{
		auto sq = no_square;

		auto attack = pos.attack_from<pt_king>(pos.king(me));
		attack |= pawn_attack<me>(pos.pieces(me, pt_pawn));

		const auto* p_square = pos.piece_list(me, pt_knight);
		while ((sq = *p_square++) != no_square)
			attack |= pos.attack_from<pt_knight>(sq);

		p_square = pos.piece_list(me, pt_bishop);
		while ((sq = *p_square++) != no_square)
			attack |= pos.attack_from<pt_bishop>(sq);

		p_square = pos.piece_list(me, pt_rook);
		while ((sq = *p_square++) != no_square)
			attack |= pos.attack_from<pt_rook>(sq);

		p_square = pos.piece_list(me, pt_queen);
		while ((sq = *p_square++) != no_square)
			attack |= pos.attack_from<pt_queen>(sq);

		return attack;
	}

	// calculate scale factor based on material hash, opposite colored bishops, passed pawns, etc 
	sfactor calculate_scale_factor(const position& pos, const material::mat_hash_entry* material_entry, const int value)
	{
		const auto strong_side = value > draw_eval ? white : black;
		auto scale_factor = material_entry->scale_factor_from_function(pos, strong_side);

		if (abs(value) <= bishop_eval && (scale_factor == normal_factor || scale_factor == one_pawn_factor))
		{
			if (pos.different_color_bishops())
			{
				if (pos.non_pawn_material(white) == mat_bishop
					&& pos.non_pawn_material(black) == mat_bishop)
					scale_factor = pos.number(strong_side, pt_pawn) > 1 ? static_cast<sfactor>(50) : static_cast<sfactor>(12);
				else
					scale_factor = static_cast<sfactor>(scale_factor * sf_mult / sf_div);
			}
			else if (pos.number(strong_side, pt_pawn) <= 2
				&& !pos.is_passed_pawn(~strong_side, pos.king(~strong_side)))
				scale_factor = static_cast<sfactor>(58 + 11 * pos.number(strong_side, pt_pawn));
		}

		return scale_factor;
	}

	int eval_after_null_move(const int eval)
	{
		const auto result = -eval + 2 * value_tempo;
		return result;
	}

	template <side me>
	inline void eval_init(const position& pos, attack_info& ai, const pawn::pawn_hash_entry* pawn_entry)
	{
		ai.attack[me][pt_king] = pos.attack_from<pt_king>(pos.king(me));
		ai.attack[me][pt_pawn] = pawn_entry->pawn_attack(me);
		ai.attack[me][pt_knight] = 0;
		ai.attack[me][pt_bishop] = 0;
		ai.attack[me][pt_rook] = 0;
		ai.attack[me][pt_queen] = 0;
		ai.attack[me][pieces_without_king] = ai.attack[me][pt_pawn];
		ai.double_attack[me] = 0;

		ai.pinned[me] = pos.info()->x_ray[me];
		ai.k_zone[me] = king_zone[pos.king(me)];

		ai.k_attack_score[me] = 0;
	}

	template <side me>
	inline int eval_bishops(const position& pos, attack_info& ai, const pawn::pawn_hash_entry* pawn_entry)
	{
		constexpr auto you = me == white ? black : white;
		auto score = 0;
		auto squares = pos.pieces(me, pt_bishop);
		assert(squares);

		if (shift_up<me>(pos.pieces(me, pt_king)) & squares)
			score += bishop_in_front_of_king;
		if constexpr (me == white)
		{
			if (squares & bb2(a1, h1))
			{
				// white bishop fianchetto bonus
				if (pos.pieces(me) & squares << 9 & b2)
					score -= bishop_in_corner;
				if (pos.pieces(me) & squares << 7 & g2)
					score -= bishop_in_corner;
			}
		}
		else
		{
			if (squares & bb2(a8, h8))
			{
				// black bishop fianchetto bonus
				if (pos.pieces(me) & squares >> 7 & b7)
					score -= bishop_in_corner;
				if (pos.pieces(me) & squares >> 9 & g7)
					score -= bishop_in_corner;
			}
		}

		do
		{
			const auto sq = pop_lsb(&squares);
			score += distance_b_k[distance(sq, pos.king(me))];

			// bishop pins queen or rook
			auto bb_pin_rq = empty_attack[pt_bishop][sq] & pos.pieces(you, pt_rook, pt_queen);
			while (bb_pin_rq)
			{
				const auto square_rq = pop_lsb(&bb_pin_rq);
				if (const auto b = bb_between(square_rq, sq) & pos.pieces(); b && !more_than_one(b))
					score += bishop_pin[me][pos.piece_on_square(lsb(b))];
			}

			auto attack = attack_bb_bishop(sq, pos.pieces(pt_pawn));
			score += mobility_b1[(popcnt(attack) * mob_mult_b1[relative_square(me, sq)] + 16) / 32];

			// bishop trapped underneath pawns
			if (pos.pieces(pt_pawn) & trapped_bishop_b3_c2[me][sq])
			{
				if (pos.pieces(pt_pawn) & trapped_bishop_b3_c2_extra[sq])
					score -= trapped_bishop_extra;
				else
					score -= trapped_bishop;
			}

			attack = attack_bb_bishop(sq, pos.pieces() ^ pos.pieces(me, pt_queen));
			if (attack & ai.k_zone[you])
				ai.k_attack_score[me] += k_zone_attack_bonus;
			ai.attack[me][pt_bishop] |= attack;
			ai.double_attack[me] |= ai.attack[me][pieces_without_king] & attack;
			ai.attack[me][pieces_without_king] |= attack;

			attack &= ai.mobility_mask[me];
			if (ai.pinned[me] & sq)
				attack &= bb_between(pos.king(me), sq);
			const uint32_t mobility = popcnt(attack);
			score += mobility_b2[(mobility * mob_mult_b2[relative_square(me, sq)] + 16) / 32];

			const auto pawns_on_color = pawn_on_color_bishop[pawn_entry->pawns_on_color(me, sq)];
			score += pawns_on_color;
			score += pawn_other_color_bishop[pawn_entry->pawns_not_on_color(me, sq)];

			if (const auto squares_same_color = dark_squares & sq ? dark_squares : ~dark_squares; pos.pieces(me, pt_bishop)
				& ~squares_same_color && !(squares_same_color & pos.pieces(you, pt_bishop)))
				score += pawns_on_color;

			if (pos.pieces(you, pt_knight) & bb_b_dominates_p[me][sq])
				score += bishop_dominates_pawn;
		} while (squares);

		return score;
	}

	inline int eval_initiative(const position& pos, const pawn::pawn_hash_entry* pawn_entry, const int eg)
	{
		const auto k_distance = file_distance(pos.king(white), pos.king(black)) - rank_distance(pos.king(white), pos.king(black));
		const auto initiative = (2 * pawn_entry->asymmetry + k_distance + 3 * pos.number(eg < 0 ? b_pawn : w_pawn) - 15) * initiative_mult;
		const auto bonus = ((eg > 0) - (eg < 0)) * std::max(initiative, -abs(eg >> 1));

		return bonus;
	}

	template <side me>
	inline int eval_king_attack(const position& pos, const attack_info& ai, const int attack_score)
	{
		const auto you = me == white ? black : white;
		auto attack_index = attack_score;
		attack_index += k_attack_index_factor * popcnt(ai.attack[you][pt_king] & ai.attack[me][all_pieces] & ~ai.attack[you][pieces_without_king]);

		if (ai.pinned[you])
			attack_index += k_attack_pin_factor;

		attack_index += k_attack_sd_factor * popcnt(shift_down<me>(ai.attack[you][pt_king]) & ~ai.attack[you][all_pieces] & ai.attack[me][all_pieces] & ~pos.pieces(me));

		const auto square_k = pos.king(you);
		const auto check_ok = ~pos.pieces(me);

		const auto check_squares_r = pos.attack_from<pt_rook>(square_k) & check_ok;
		const auto check_squares_b = pos.attack_from<pt_bishop>(square_k) & check_ok;
		const auto check_squares_p = pos.attack_from<pt_knight>(square_k) & check_ok;

		const auto safe_pbr = ~ai.attack[you][all_pieces] | (ai.double_attack[me] & (~ai.double_attack[you] & (ai.attack[you][pt_king] | ai.attack[you][pt_queen])));

		if (check_squares_p & ai.attack[me][pt_knight])
		{
			if (check_squares_p & ai.attack[me][pt_knight] & safe_pbr)
				attack_index += cspan_safe;
			else
				attack_index += cspan;
		}
		if (check_squares_b & ai.attack[me][pt_bishop])
		{
			if (check_squares_b & ai.attack[me][pt_bishop] & safe_pbr)
				attack_index += csbab_safe;
			else
				attack_index += csbab;
		}
		if (check_squares_r & ai.attack[me][pt_rook])
		{
			if (check_squares_r & ai.attack[me][pt_rook] & safe_pbr)
				attack_index += csrar_safe;
			else
				attack_index += csrar;
		}

		if (auto queen_check = (check_squares_b | check_squares_r) & ai.attack[me][pt_queen]; queen_check)
		{
			if (queen_check & ~ai.attack[you][pt_king])
			{
				if (queen_check & ~ai.attack[you][all_pieces])
					attack_index += qcayk_all;
				else
					attack_index += qcayk;
			}
			queen_check &= ai.attack[you][pt_king] & (pos.pieces(you, pt_queen) | ai.double_attack[me]) & ~ai.attack[you][pieces_without_king];
			if (queen_check)
				attack_index += queen_check_bonus;
		}

		if (attack_index < 0)
			attack_index = 0;
		if (attack_index > 1000)
			attack_index = 1000;

		return safety_table[attack_index];
	}

	template <side me>
	inline int eval_knights(const position& pos, attack_info& ai)
	{
		constexpr auto you = me == white ? black : white;
		auto score = 0;
		auto squares = pos.pieces(me, pt_knight);
		assert(squares);
		do
		{
			const auto sq = pop_lsb(&squares);
			score += distance_p_k[distance(sq, pos.king(me))];
			auto attack = pos.attack_from<pt_knight>(sq);
			if (attack & ai.k_zone[you])
				ai.k_attack_score[me] += knight_attack_king;
			ai.attack[me][pt_knight] |= attack;
			ai.double_attack[me] |= ai.attack[me][pieces_without_king] & attack;
			ai.attack[me][pieces_without_king] |= attack;

			uint32_t mobility = 0;
			if (ai.pinned[me] & sq)
				mobility = 0;
			else
			{
				attack &= ai.mobility_mask[me];
				mobility = popcnt(attack) + popcnt(attack & bb_ranks_forward(me, sq));
			}
			score += mobility_p[(mobility * mob_mult_p[relative_square(me, sq)] + p_mobility_add) / p_mobility_div];
		} while (squares);

		return score;
	}

	template <side me>
	inline int eval_passed_pawns(const position& pos, const attack_info& ai, uint64_t bb_passed_pawns)
	{
		assert(bb_passed_pawns != 0);
		constexpr auto you = me == white ? black : white;
		auto score = 0;
		while (bb_passed_pawns)
		{
			const auto passed_pawn = pop_lsb(&bb_passed_pawns);

			const auto pawn_rank = relative_rank(me, passed_pawn) - 1;
			if (pos.non_pawn_material(white) == mat_queen && pos.non_pawn_material(black) == mat_queen)
				score += passed_pawn_dvd[pawn_rank];
			else
				score += passed_pawn_not_dvd[pawn_rank];

			if (pawn_rank > 1)
			{
				const auto square_for_pawn = passed_pawn + pawn_ahead(me);
				auto my_distance = distance(square_for_pawn, pos.king(me));
				auto your_distance = distance(square_for_pawn, pos.king(you));

				score += mul_div(passed_pawn_my_k[pawn_rank][my_distance], passed_pawn_mk_mult, passed_pawn_mk_div);
				score += mul_div(passed_pawn_your_k[pawn_rank][your_distance], passed_pawn_yk_mult, passed_pawn_yk_div);

				const auto promotion_square = make_square(file_of(passed_pawn), static_cast<rank>(7 * you));
				my_distance = distance(promotion_square, pos.king(me));
				your_distance = distance(promotion_square, pos.king(you));

				score += mul_div(passed_pawn_my_k[pawn_rank][my_distance], passed_pawn_mk_md_mul, passed_pawn_mk_md_div);
				score += mul_div(passed_pawn_your_k[pawn_rank][your_distance], passed_pawn_yk_md_mul, passed_pawn_yk_md_div);

				if (pawn_rank > 2)
				{
					const auto bb_behind_passed_pawn = bb_forward(you, passed_pawn);
					if (bb_behind_passed_pawn & pos.pieces(me, pt_rook))
						score += bb_behind_passed_pawn_bonus;
					if (bb_behind_passed_pawn & pos.pieces(you, pt_rook))
						score -= bb_behind_passed_pawn_bonus;
				}

				if (!(pos.pieces() & square_for_pawn))
				{
					const auto passed_pawn_path = bb_forward(me, passed_pawn);
					auto bb_advance_blocked = passed_pawn_path & (pos.pieces(you) | ai.attack[you][all_pieces]);

					if (const auto attacked = pos.pieces(you, pt_rook, pt_queen) & bb_forward(you, passed_pawn); attacked)
					{
						if (const auto sq = front_square(me, attacked); !(pos.pieces() & bb_between(passed_pawn, sq)))
							bb_advance_blocked = passed_pawn_path;
					}

					if (!bb_advance_blocked)
						score += passed_pawn_free_passage[pawn_rank];
					else if (bb_advance_blocked & ~ai.attack[me][all_pieces])
						score += passed_pawn_advance_blocked[pawn_rank];
					else
						score += passed_pawn_advance_supported[pawn_rank];
				}
			}
		}

		return score;
	}

	template <side me>
	inline int eval_queens(const position& pos, attack_info& ai)
	{
		constexpr auto you = me == white ? black : white;
		auto score = 0;
		auto squares = pos.pieces(me, pt_queen);
		assert(squares);

		const auto mobility_mask_d = ~(ai.attack[you][pt_bishop] | ai.attack[you][pt_rook]
			| pos.pieces(me, pt_king, pt_pawn)
			| ai.attack[you][pt_pawn]
			| ai.attack[you][pt_knight]);
		ai.k_attack_score[me] += queen_attack_king;

		do
		{
			const auto sq = pop_lsb(&squares);
			auto attack = attack_bb_rook(sq, pos.pieces() ^ pos.pieces(me, pt_queen))
				| attack_bb_bishop(sq, pos.pieces() ^ pos.pieces(me, pt_queen));
			if (attack & ai.k_zone[you])
				ai.k_attack_score[me] += queen_attack_king_zone;
			ai.attack[me][pt_queen] |= attack;
			ai.double_attack[me] |= ai.attack[me][pieces_without_king] & attack;
			ai.attack[me][pieces_without_king] |= attack;

			attack &= mobility_mask_d;
			if (ai.pinned[me] & sq)
				attack &= bb_between(pos.king(me), sq);

			constexpr uint64_t center_square = 0x00003C3C3C3C0000;
			const uint32_t mobility = popcnt(attack) + popcnt(attack & center_square);
			score += mobility_q[(mobility * mob_mult_q[relative_square(me, sq)] + q_mobility_add) / q_mobility_div];
		} while (squares);

		return score;
	}

	template <side me>
	inline int eval_rooks(const position& pos, attack_info& ai)
	{
		constexpr auto you = me == white ? black : white;
		auto score = 0;
		auto squares = pos.pieces(me, pt_rook);
		assert(squares);

		if (pos.pieces(me, pt_king) & (me == white ? bb2(f1, g1) : bb2(f8, g8)) && squares & (me == white ? 0xC0C0 : 0xC0C0000000000000))
			score -= uncastled_penalty;
		do
		{
			const auto sq = pop_lsb(&squares);

			auto attack = attack_bb_rook(sq, pos.pieces() ^ pos.pieces(me, pt_rook, pt_queen));
			if (attack & ai.k_zone[you])
				ai.k_attack_score[me] += rook_attacks_king;
			ai.attack[me][pt_rook] |= attack;
			ai.double_attack[me] |= ai.attack[me][pieces_without_king] & attack;
			ai.attack[me][pieces_without_king] |= attack;

			attack &= ai.mobility_mask[me];
			if (ai.pinned[me] & sq)
				attack &= bb_between(pos.king(me), sq);
			const uint32_t mobility = popcnt(attack);
			score += mobility_r[(mobility * mob_mult_r[relative_square(me, sq)] + r_mobility_add) / r_mobility_div];

			if (constexpr auto eighth_rank = me == white ? rank_8_bb : rank_1_bb; relative_rank(me, sq) == rank_7 && pos.pieces(you, pt_king) & eighth_rank)
				score += rook_traps_king_on_7th;

			if (file_of(sq) == file_of(pos.king(you)) && !(pos.pieces(me, pt_pawn) & bb_between(pos.king(you), sq)))
				ai.k_attack_score[me] += 2 * 8;

			if (!(bb_file(sq) & pos.pieces(me, pt_pawn)))
			{
				if (const auto pawn = pos.pieces(you, pt_pawn) & bb_file(sq); !pawn)
					score += no_pawn;
				else if (pawn & ai.attack[you][pt_pawn])
					score += pawn_attacks;
				else
					score += pawn_no_attack;
			}
		} while (squares);

		return score;
	}

	template <side me>
	inline int eval_space(const position& pos, const attack_info& ai)
	{
		constexpr auto you = me == white ? black : white;
		constexpr uint64_t center_zone = me == white ? 0x000000003c3c3c00 : 0x003c3c3c00000000;
		const auto safe = center_zone & ~(pos.pieces(me, pt_pawn) | ai.attack[you][pt_pawn] | (ai.attack[you][all_pieces] & ~ai.attack[me][all_pieces]));

		auto shielded = pos.pieces(me, pt_pawn);
		shielded |= me == white ? shielded >> 8 : shielded << 8;
		shielded |= me == white ? shielded >> 16 : shielded << 16;
		shielded &= safe;

		const uint32_t control = popcnt(safe | (me == white ? shielded << 32 : shielded >> 32));
		const uint32_t weight = popcnt(pos.pieces(me));
		const auto escore = remake_score(weight * weight * space_weight_mult / space_weight_div, eval_0);
		return escore * control;
	}

	template <side me>
	inline int eval_strong_squares(const position& pos, const attack_info& ai, const pawn::pawn_hash_entry* pawn_entry)
	{
		constexpr auto you = me == white ? black : white;
		constexpr uint64_t rank456 = me == white ? 0x3C3C3C000000 : 0x3C3C3C0000;
		auto score = 0;
		score += safety_for_pawn_rbp * popcnt(pawn_entry->safe_for_pawn(you) & pos.pieces(me, pt_knight, pt_bishop, pt_rook));
		score += strong_p_in_front_of_pawn * popcnt(
			pawn_entry->safe_for_pawn(you) & pos.pieces(me, pt_knight) & shift_down<me>(pos.pieces(you, pt_pawn)));

		auto strong_pb = pawn_entry->safe_for_pawn(you) & ai.attack[me][pt_pawn] & pos.pieces(me, pt_knight, pt_bishop) & rank456;
		if (strong_pb)
		{
			score += strong_square_pb * popcnt(strong_pb);
			if (!pos.pieces(you, pt_knight))
			{
				do
				{
					const auto sq = pop_lsb(&strong_pb);
					if (const auto squares_same_color = dark_squares & sq ? dark_squares : ~dark_squares; !(pos.pieces(you, pt_bishop) & squares_same_color))
						score += strong_square_pb_extra;
				} while (strong_pb);
			}
		}

		score += pb_behind_pawn * popcnt(pos.pieces(me, pt_knight) & pawn_attack<you>(pos.pieces(me, pt_pawn)));
		score += pb_behind_pawn * popcnt(pos.pieces(me, pt_knight, pt_bishop) & shift_down<me>(pos.pieces(me, pt_pawn)));
		score += protected_piece * popcnt(pos.pieces_excluded(me, pt_pawn) & ai.attack[me][pt_pawn]);

		return score;
	}

	template <side me>
	int eval_threats(const position& pos, attack_info& ai)
	{
		constexpr auto you = me == white ? black : white;
		constexpr auto rank2 = me == white ? rank_2_bb : rank_7_bb;
		constexpr auto rank7 = me == white ? rank_7_bb : rank_2_bb;
		auto score = 0;
		ai.strong_threat[me] = false;

		if (auto pawn_threats = pos.pieces_excluded(you, pt_pawn) & ai.attack[me][pt_pawn]; pawn_threats)
		{
			const auto safe_pawns = pos.pieces(me, pt_pawn) & (~ai.attack[you][all_pieces] | ai.attack[me][all_pieces]);
			uint64_t safe_threats = pawn_attack<me>(safe_pawns) & pawn_threats;

			if (pawn_threats ^ safe_threats)
				score += hanging_pawn_threat;

			if (safe_threats)
			{
				ai.strong_threat[me] = true;
			}

			while (safe_threats)
				score += pawn_threat[piece_type(pos.piece_on_square(pop_lsb(&safe_threats)))];
		}

		const auto supported_pieces = pos.pieces_excluded(you, pt_pawn) & ai.attack[you][pt_pawn] & ai.attack[me][all_pieces];

		if (const auto weak_pieces = pos.pieces(you) & ~ai.attack[you][pt_pawn] & ai.attack[me][all_pieces]; supported_pieces | weak_pieces)
		{
			auto b = (supported_pieces | weak_pieces) & (ai.attack[me][pt_knight] | ai.attack[me][pt_bishop]);
			if (b & pos.pieces(you, pt_rook, pt_queen))
				ai.strong_threat[me] = true;
			while (b)
				score += piece_threat[piece_type(pos.piece_on_square(pop_lsb(&b)))];

			b = (pos.pieces(you, pt_queen) | weak_pieces) & ai.attack[me][pt_rook];
			if (b & pos.pieces(you, pt_queen))
				ai.strong_threat[me] = true;
			while (b)
				score += rook_threat[piece_type(pos.piece_on_square(pop_lsb(&b)))];

			b = weak_pieces & ~ai.attack[you][all_pieces];
			if (b & pos.pieces_excluded(you, pt_pawn))
				ai.strong_threat[me] = true;
			score += hanging_pieces * popcnt(b);

			b = weak_pieces & ai.attack[me][pt_king];
			if (b)
				score += more_than_one(b) ? king_threat_multiple : king_threat_single;
		}

		auto b = pos.pieces(me, pt_pawn) & ~rank7;
		b = shift_up<me>(b | (shift_up<me>(b & rank2) & ~pos.pieces()));

		b &= ~pos.pieces()
			& ~ai.attack[you][pt_pawn]
			& (ai.attack[me][all_pieces] | ~ai.attack[you][all_pieces]);

		b = pawn_attack<me>(b)
			& pos.pieces(you)
			& ~ai.attack[me][pt_pawn];

		score += pawn_advance * popcnt(b);

		return score;
	}

	int eval(const position& pos, const int alpha, const int beta)
	{
		if (pos.is_in_check())
			return score_0;
		const auto blocked_pawns = mul_div(make_score(blocked_pawns_mg, blocked_pawns_eg), 128, 256);

		auto* const material_entry = material::probe(pos);
		auto* pi = pos.info();
		pi->eval_is_exact = false;

		if (material_entry->has_value_function())
		{
			pi->strong_threat = 0;
			return material_entry->value_from_function(pos);
		}

		if (const auto do_lazy_eval = beta < win_score && (pi - 1)->eval_positional != no_eval && alpha > -win_score
			&& pos.non_pawn_material(white) + pos.non_pawn_material(black) > 2 * mat_bishop
			&& !(pos.pieces(white, pt_pawn) & rank_7_bb) && !(pos.pieces(black, pt_pawn) & rank_2_bb); do_lazy_eval)
		{
			auto val = (pi - 1)->eval_positional;
			const int eval_factor = (pi - 1)->eval_factor;
			val += material_entry->value * eval_factor / max_factor;

			if (pos.on_move() == black)
				val = -val;

			if (const auto lazy_result = val / eval_value_div + value_tempo; lazy_result <= alpha || lazy_result >= beta)
			{
				pi->strong_threat = 0;
				return lazy_result;
			}
		}

		auto* pawn_entry = pawn::probe(pos);

		auto king_safety = pawn_entry->king_safety<white>(pos);
		king_safety -= pawn_entry->king_safety<black>(pos);

		attack_info ai{};

		eval_init<white>(pos, ai, pawn_entry);
		eval_init<black>(pos, ai, pawn_entry);

		ai.mobility_mask[white] = ~(ai.attack[black][pt_pawn] | (pos.pieces(white, pt_pawn) & shift_down<white>(pos.pieces())))
			| pos.pieces_excluded(black, pt_pawn);
		ai.mobility_mask[black] = ~(ai.attack[white][pt_pawn] | (pos.pieces(black, pt_pawn) & shift_down<black>(pos.pieces())))
			| pos.pieces_excluded(white, pt_pawn);

		auto eval_score = pos.psq_score();

		if (pos.pieces(white, pt_knight))
			eval_score += eval_knights<white>(pos, ai);
		if (pos.pieces(black, pt_knight))
			eval_score -= eval_knights<black>(pos, ai);

		if (pos.pieces(white, pt_bishop))
			eval_score += eval_bishops<white>(pos, ai, pawn_entry);
		if (pos.pieces(black, pt_bishop))
			eval_score -= eval_bishops<black>(pos, ai, pawn_entry);

		if (pos.pieces(white, pt_rook))
			eval_score += eval_rooks<white>(pos, ai);
		if (pos.pieces(black, pt_rook))
			eval_score -= eval_rooks<black>(pos, ai);

		if (pos.pieces(white, pt_queen))
			eval_score += eval_queens<white>(pos, ai);
		if (pos.pieces(black, pt_queen))
			eval_score -= eval_queens<black>(pos, ai);

		ai.double_attack[white] |= ai.attack[white][pieces_without_king] & ai.attack[white][pt_king];
		ai.attack[white][all_pieces] = ai.attack[white][pieces_without_king] | ai.attack[white][pt_king];
		ai.double_attack[black] |= ai.attack[black][pieces_without_king] & ai.attack[black][pt_king];
		ai.attack[black][all_pieces] = ai.attack[black][pieces_without_king] | ai.attack[black][pt_king];

		eval_score += eval_king_attack<white>(pos, ai, ai.k_attack_score[white] - pawn_entry->safety[black]);
		eval_score -= eval_king_attack<black>(pos, ai, ai.k_attack_score[black] - pawn_entry->safety[white]);

		eval_score += eval_threats<white>(pos, ai);
		eval_score -= eval_threats<black>(pos, ai);
		pi->strong_threat = ai.strong_threat[white] + 2 * ai.strong_threat[black];

		if (pawn_entry->passed_pawns(white))
			eval_score += eval_passed_pawns<white>(pos, ai, pawn_entry->passed_pawns(white));
		if (pawn_entry->passed_pawns(black))
			eval_score -= eval_passed_pawns<black>(pos, ai, pawn_entry->passed_pawns(black));

		eval_score += eval_strong_squares<white>(pos, ai, pawn_entry);
		eval_score -= eval_strong_squares<black>(pos, ai, pawn_entry);

		eval_score -= blocked_pawns * popcnt(pos.pieces(white, pt_pawn) & shift_down<white>(pos.pieces()));
		eval_score += blocked_pawns * popcnt(pos.pieces(black, pt_pawn) & shift_down<black>(pos.pieces()));
		
		if (pos.non_pawn_material(white) + pos.non_pawn_material(black) >= space_threshold)
		{
			eval_score += eval_space<white>(pos, ai);
			eval_score -= eval_space<black>(pos, ai);
		}

		auto bb_flank = ai.attack[white][pieces_without_king] & bb_king_flank_attack[black][file_of(pos.king(black))]
			& ~ai.attack[black][pt_king] & ~ai.attack[black][pt_pawn];
		bb_flank = bb_flank >> 4 | (bb_flank & ai.double_attack[white]);
		eval_score += popcnt(bb_flank) * flank_double_attack;

		bb_flank = ai.attack[black][pieces_without_king] & bb_king_flank_attack[white][file_of(pos.king(white))]
			& ~ai.attack[white][pt_king] & ~ai.attack[white][pt_pawn];
		bb_flank = bb_flank << 4 | (bb_flank & ai.double_attack[black]);
		eval_score -= popcnt(bb_flank) * flank_double_attack;

		auto score = pawn_entry->pawns_score() + king_safety + mul_div(eval_score, eval_mult, eval_div);

		if (pos.non_pawn_material(white) + pos.non_pawn_material(black) <= 4 * mat_bishop)
			score += pawn_file_width[pawn_entry->pawn_range(white)] - pawn_file_width[pawn_entry->pawn_range(black)];

		const auto mg = (mg_mgvalue_mult * mg_value(score) - mg_egvalue_mult * eg_value(score)) / 100;
		auto eg = (eg_mgvalue_mult * mg_value(score) + eg_egvalue_mult * eg_value(score)) / 100;

		const auto scale_factor = calculate_scale_factor(pos, material_entry, material_entry->value + eg);
		auto conversion = material_entry->conversion;

		eg += eval_initiative(pos, pawn_entry, material_entry->value + eg);

		if (material_entry->conversion_is_estimated && !pos.pieces(pt_queen) && !(pawn_entry->passed_pawns(white) | pawn_entry->passed_pawns(black)))
			conversion = static_cast<sfactor>(conversion * conversion_mult / conversion_div);
		if (pawn_entry->conversion_difficult)
			conversion = static_cast<sfactor>(conversion * conversion_mult / conversion_div);

		const auto eval_factor = scale_factor == normal_factor
			? conversion
			: static_cast<sfactor>(std::min(static_cast<int>(conversion), 2 * scale_factor));

		const auto phase = material_entry->get_game_phase();
		
		auto val = (mg * conversion / max_factor * phase + eg * eval_factor / max_factor * (middlegame_phase - phase)) /
			static_cast<int>(middlegame_phase);
		
		pi->eval_positional = val;
		pi->eval_factor = static_cast<uint8_t>(eval_factor);
		val += material_entry->value * eval_factor / max_factor;

		if (thread_pool.piece_contempt)
		{
			const auto contempt_number = pawn_contempt_mult * pos.number(thread_pool.contempt_color, pt_pawn)
				+ knight_contempt_mult * pos.number(thread_pool.contempt_color, pt_knight) + bishop_contempt_mult * pos.number(thread_pool.contempt_color, pt_bishop)
				+ rook_contempt_mult * pos.number(thread_pool.contempt_color, pt_rook) + queen_contempt_mult * pos.number(thread_pool.contempt_color, pt_queen);

			if (const auto contempt_score = contempt_mult * thread_pool.piece_contempt * contempt_number * eval_factor / max_factor; thread_pool.contempt_color == white)
				val += static_cast<int>(contempt_score);
			else
				val -= static_cast<int>(contempt_score);
		}

		if (pos.on_move() == black)
			val = -val;

		auto result = val / eval_value_div + value_tempo;

		if (pos.fifty_move_counter() > thread_pool.fifty_move_distance)
			result = result * (5 * (2 * thread_pool.fifty_move_distance - pos.fifty_move_counter()) + 6) / 256;

		if (!pos.non_pawn_material(pos.on_move()))
		{
			if (pos.on_move() == white)
			{
				if ((pos.attack_from<pt_king>(pos.king(white)) & ~pos.pieces(white) & ~ai.attack[black][all_pieces]) == 0
					&& (pos.pieces(white, pt_pawn) << 8 & ~pos.pieces()) == 0
					&& ((pos.pieces(white, pt_pawn) & ~file_a_bb) << 7 & pos.pieces(black)) == 0
					&& ((pos.pieces(white, pt_pawn) & ~file_h_bb) << 9 & pos.pieces(black)) == 0)
				{
					result = draw_score;
					pi->eval_is_exact = true;
				}
			}
			else if ((pos.attack_from<pt_king>(pos.king(black)) & ~pos.pieces(black) & ~ai.attack[white][all_pieces]) == 0
				&& (pos.pieces(black, pt_pawn) >> 8 & ~pos.pieces()) == 0
				&& ((pos.pieces(black, pt_pawn) & ~file_a_bb) >> 9 & pos.pieces(white)) == 0
				&& ((pos.pieces(black, pt_pawn) & ~file_h_bb) >> 7 & pos.pieces(white)) == 0)
			{
				result = draw_score;
				pi->eval_is_exact = true;
			}
		}
		return result;
	}

	template <side me>
	bool two_mobile_pieces(const position& pos)
	{
		constexpr auto you = me == white ? black : white;
		auto sq = no_square;

		const uint64_t your_attack = calculate_attack<you>(pos);
		const auto pinned = pos.info()->x_ray[me];

		const auto safe = ~pos.pieces(me) & ~your_attack;
		auto mobile = false;

		if (const auto attack = pos.attack_from<pt_king>(pos.king(me)); attack & safe)
		{
			if (!(attack & pos.pieces(me) & your_attack))
				mobile = true;
		}

		const auto* p_square = pos.piece_list(me, pt_knight);
		while ((sq = *p_square++) != no_square)
		{
			if (pinned & sq)
				continue;
			if (pos.attack_from<pt_knight>(sq) & safe)
			{
				if (mobile)
					return true;
				mobile = true;
			}
		}

		p_square = pos.piece_list(me, pt_bishop);
		while ((sq = *p_square++) != no_square)
		{
			if (pinned & sq)
				continue;
			if (pos.attack_from<pt_bishop>(sq) & safe)
			{
				if (mobile)
					return true;
				mobile = true;
			}
		}

		p_square = pos.piece_list(me, pt_rook);
		while ((sq = *p_square++) != no_square)
		{
			if (pinned & sq)
				continue;
			if (pos.attack_from<pt_rook>(sq) & safe)
			{
				if (mobile)
					return true;
				mobile = true;
			}
		}

		p_square = pos.piece_list(me, pt_queen);
		while ((sq = *p_square++) != no_square)
		{
			if (pinned & sq)
				continue;
			if (pos.attack_from<pt_queen>(sq) & safe)
			{
				if (mobile)
					return true;
				mobile = true;
			}
		}

		return false;
	}

	bool has_two_mobile_pieces(const position& pos)
	{
		if (pos.on_move() == white)
			return two_mobile_pieces<white>(pos);
		return two_mobile_pieces<black>(pos);
	}
}