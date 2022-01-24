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

// this tuner uses Texel Peter Österlund tuning method
// https://www.chessprogramming.org/Texel%27s_Tuning_Method
// the routines are adapted from the implementation found in
// Monolith https://github.com/cimarronOST/Monolith

#include "../fire.h"

#ifdef TUNER
#include "tune.h"

#include <atomic>
#include <thread>
#include <cmath>
#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <chrono>
#include <limits>
#include <algorithm>

#include "evaluate.h"
#include "material.h"
#include "position.h"
#include "thread.h"
#include "util.h"

namespace tuner
{
	constexpr int k_precision = 5;
	constexpr int weight_cnt = 1163;
	int calc_errors{};
	struct texel_position
	{
		position pos;
		double result;
	};

	int load_pos(std::string& epd_file, std::vector<texel_position>& texel_pos)
	{
		// load the positions from the epd-file

		std::fstream file;
		file.open(epd_file, std::ios::in);

		// file could not be opened
		if (file.fail())
			acout() << "" << epd_file << " not found" << std::endl;

		std::string fen_str;

		if (file.is_open())
		{
			while (getline(file, fen_str))
			{
				texel_position curr_pos{};

				const auto fen_end
				{
					fen_str.find_last_of(' ', std::string::npos)
				};
				auto result
				{
					fen_str.substr(fen_end + 1, std::string::npos)
				};

				if (result == "1/2-1/2") curr_pos.result = 0.5;
				else if (result == "1-0") curr_pos.result = 1.0;
				else if (result == "0-1") curr_pos.result = 0.0;
				//acout() << result << std::endl;
				//acout() << curr_pos.result << std::endl;

				curr_pos.pos.set(fen_str, false, thread_pool.main());

				//acout() << fen_str.c_str() << std::endl;
				//acout() << curr_pos.pos << std::endl;

				//const auto alpha = -30256;
				//const auto beta = 30256;
				//auto const depth = static_cast<e_depth>(0);
				//auto val = search::q_search<search::node_type::PV, false>(curr_pos.pos, alpha, beta, depth);

				texel_pos.push_back(curr_pos);
			}
			file.close();
		}
		return 0;
	}

	void load_weights(std::vector<int>& weights)
	{
		// init mobility mult
		weights.push_back(evaluate::mob_factor_p);
		weights.push_back(evaluate::mmrq_factor_p);
		weights.push_back(evaluate::mmfq_factor_p);
		weights.push_back(evaluate::mmc_factor_p);
		weights.push_back(evaluate::mmr_factor_p);
		weights.push_back(evaluate::mme_factor_p);

		weights.push_back(evaluate::mob_factor_b1);
		weights.push_back(evaluate::mob_factor_b2);
		weights.push_back(evaluate::mmrq_factor_b2);
		weights.push_back(evaluate::mmfq_factor_b2);
		weights.push_back(evaluate::mmc_factor_b2);
		weights.push_back(evaluate::mmr_factor_b2);
		weights.push_back(evaluate::mme_factor_b2);

		weights.push_back(evaluate::mob_factor_r);
		weights.push_back(evaluate::mmrq_factor_r);
		weights.push_back(evaluate::mmfq_factor_r);
		weights.push_back(evaluate::mmc_factor_r);
		weights.push_back(evaluate::mmr_factor_r);
		weights.push_back(evaluate::mme_factor_r);

		weights.push_back(evaluate::mob_factor_q);
		weights.push_back(evaluate::mmrq_factor_q);
		weights.push_back(evaluate::mmfq_factor_q);
		weights.push_back(evaluate::mmc_factor_q);
		weights.push_back(evaluate::mmr_factor_q);
		weights.push_back(evaluate::mme_factor_q);

		// init pawn mobility values
		weights.push_back(static_cast<int>(evaluate::pawn_mg_mult));
		weights.push_back(static_cast<int>(evaluate::pawn_mg_sub));
		weights.push_back(static_cast<int>(evaluate::pawn_eg_mult));
		weights.push_back(static_cast<int>(evaluate::pawn_eg_sub));

		// init bishop mobility values
		weights.push_back(static_cast<int>(evaluate::b1_mg_mult));
		weights.push_back(static_cast<int>(evaluate::b1_mg_sub));
		weights.push_back(static_cast<int>(evaluate::b1_eg_mult));
		weights.push_back(static_cast<int>(evaluate::b1_eg_sub));

		weights.push_back(static_cast<int>(evaluate::b2_mg_mult));
		weights.push_back(static_cast<int>(evaluate::b2_mg_sub));
		weights.push_back(static_cast<int>(evaluate::b2_eg_mult));
		weights.push_back(static_cast<int>(evaluate::b2_eg_sub));

		// init rook mobility values
		weights.push_back(static_cast<int>(evaluate::rook_mg_mult));
		weights.push_back(static_cast<int>(evaluate::rook_mg_sub));
		weights.push_back(static_cast<int>(evaluate::rook_eg_mult));
		weights.push_back(static_cast<int>(evaluate::rook_eg_sub));
		weights.push_back(static_cast<int>(evaluate::mob_r_mult));
		weights.push_back(static_cast<int>(evaluate::mob_r_div));

		// init queen mobility values
		weights.push_back(static_cast<int>(evaluate::queen_mg_mult));
		weights.push_back(static_cast<int>(evaluate::queen_mg_sub));
		weights.push_back(static_cast<int>(evaluate::queen_eg_mult));
		weights.push_back(static_cast<int>(evaluate::queen_eg_sub));

		// init distance values
		weights.push_back(evaluate::p_k_distance);
		weights.push_back(evaluate::p_k_distance_mult);
		weights.push_back(evaluate::b_k_distance);
		weights.push_back(evaluate::b_k_distance_mult);

		// init pawn-bishop color values
		weights.push_back(evaluate::pawn_on_bishop_color);
		weights.push_back(evaluate::pawn_on_other_bishop_color);
		weights.push_back(evaluate::pawn_file_width_mg);
		weights.push_back(evaluate::pawn_file_width_eg);
		weights.push_back(evaluate::threats_score);

		// init passed pawn values
		weights.push_back(evaluate::pp_dvd_mgfactor);
		weights.push_back(evaluate::pp_dvd_egfactor);
		weights.push_back(evaluate::pp_ndvd_mgfactor);
		weights.push_back(evaluate::pp_ndvd_egfactor);

		weights.push_back(evaluate::pp_fp_base_mg);
		weights.push_back(evaluate::pp_fp_base_eg);
		weights.push_back(evaluate::pp_fp_mg);
		weights.push_back(evaluate::pp_fp_eg);
		weights.push_back(evaluate::pp_fp_mul);
		weights.push_back(evaluate::pp_fp_div);

		weights.push_back(evaluate::pp_as_base_mg);
		weights.push_back(evaluate::pp_as_base_eg);
		weights.push_back(evaluate::pp_as_mg);
		weights.push_back(evaluate::pp_as_eg);
		weights.push_back(evaluate::pp_as_mul);
		weights.push_back(evaluate::pp_as_div);

		weights.push_back(evaluate::pp_ab_base_mg);
		weights.push_back(evaluate::pp_ab_base_eg);

		weights.push_back(evaluate::pp_support_proximity_factor);
		weights.push_back(evaluate::pp_mk_kdfp_factor);
		weights.push_back(evaluate::pp_yk_kdfp_factor);
		weights.push_back(evaluate::pp_mk_factor);
		weights.push_back(evaluate::pp_mk_div);
		weights.push_back(evaluate::pp_yk_factor);
		weights.push_back(evaluate::pp_yk_div);

		// calculate_scale_factor
		weights.push_back(evaluate::sf_mult);
		weights.push_back(evaluate::sf_div);

		// eval_bishops
		weights.push_back(evaluate::bishop_in_front_of_king);
		weights.push_back(evaluate::bishop_in_corner);
		weights.push_back(evaluate::trapped_bishop_extra);
		weights.push_back(evaluate::trapped_bishop);
		weights.push_back(evaluate::bishop_dominates_pawn);
		weights.push_back(evaluate::k_zone_attack_bonus);

		// eval_initiative
		weights.push_back(evaluate::initiative_mult);

		// eval_king_attack
		weights.push_back(evaluate::k_attack_index_factor);
		weights.push_back(evaluate::k_attack_pin_factor);
		weights.push_back(evaluate::k_attack_sd_factor);
		weights.push_back(evaluate::cspan_safe);
		weights.push_back(evaluate::cspan);
		weights.push_back(evaluate::csbab_safe);
		weights.push_back(evaluate::csbab);
		weights.push_back(evaluate::csrar_safe);
		weights.push_back(evaluate::csrar);
		weights.push_back(evaluate::qcayk_all);
		weights.push_back(evaluate::qcayk);
		weights.push_back(evaluate::queen_check_bonus);

		// eval_knights
		weights.push_back(evaluate::knight_attack_king);
		weights.push_back(evaluate::p_mobility_add);
		weights.push_back(evaluate::p_mobility_div);

		// eval_passed_pawns
		weights.push_back(evaluate::passed_pawn_mk_mult);
		weights.push_back(evaluate::passed_pawn_mk_div);
		weights.push_back(evaluate::passed_pawn_yk_mult);
		weights.push_back(evaluate::passed_pawn_yk_div);
		weights.push_back(evaluate::passed_pawn_mk_md_mul);
		weights.push_back(evaluate::passed_pawn_mk_md_div);
		weights.push_back(evaluate::passed_pawn_yk_md_mul);
		weights.push_back(evaluate::passed_pawn_yk_md_div);
		weights.push_back(evaluate::bb_behind_passed_pawn_bonus);

		// eval_queens
		weights.push_back(evaluate::queen_attack_king);
		weights.push_back(evaluate::queen_attack_king_zone);
		weights.push_back(evaluate::q_mobility_add);
		weights.push_back(evaluate::q_mobility_div);

		// eval_rooks
		weights.push_back(evaluate::uncastled_penalty);
		weights.push_back(evaluate::rook_attacks_king);
		weights.push_back(evaluate::rook_traps_king_on_7th);
		weights.push_back(evaluate::no_pawn);
		weights.push_back(evaluate::pawn_attacks);
		weights.push_back(evaluate::pawn_no_attack);
		weights.push_back(evaluate::r_mobility_add);
		weights.push_back(evaluate::r_mobility_div);

		// eval_space
		weights.push_back(evaluate::space_threshold);
		weights.push_back(evaluate::space_weight_mult);
		weights.push_back(evaluate::space_weight_div);

		// eval_strong_squares
		weights.push_back(evaluate::safety_for_pawn_rbp);
		weights.push_back(evaluate::strong_p_in_front_of_pawn);
		weights.push_back(evaluate::strong_square_pb);
		weights.push_back(evaluate::strong_square_pb_extra);
		weights.push_back(evaluate::pb_behind_pawn);
		weights.push_back(evaluate::protected_piece);

		// eval_threats
		weights.push_back(evaluate::hanging_pawn_threat);
		weights.push_back(evaluate::hanging_pieces);
		weights.push_back(evaluate::king_threat_single);
		weights.push_back(evaluate::king_threat_multiple);
		weights.push_back(evaluate::pawn_advance);

		// eval
		weights.push_back(evaluate::blocked_pawns_mg);
		weights.push_back(evaluate::blocked_pawns_eg);
		weights.push_back(evaluate::mg_mgvalue_mult);
		weights.push_back(evaluate::mg_egvalue_mult);
		weights.push_back(evaluate::eg_mgvalue_mult);
		weights.push_back(evaluate::eg_egvalue_mult);
		weights.push_back(evaluate::eval_mult);
		weights.push_back(evaluate::conversion_mult);
		weights.push_back(evaluate::conversion_div);
		weights.push_back(evaluate::eval_div);
		weights.push_back(evaluate::eval_value_div);
		weights.push_back(evaluate::flank_double_attack);
		weights.push_back(evaluate::pawn_contempt_mult);
		weights.push_back(evaluate::knight_contempt_mult);
		weights.push_back(evaluate::bishop_contempt_mult);
		weights.push_back(evaluate::rook_contempt_mult);
		weights.push_back(evaluate::queen_contempt_mult);
		weights.push_back(evaluate::contempt_mult);

		for (auto& w : evaluate::passed_pawn_proximity) weights.push_back(w);

		// threat arrays
		for (auto& w : evaluate::piece_threat) weights.push_back(w);
		for (auto& w : evaluate::rook_threat) weights.push_back(w);
		for (auto& w : evaluate::pawn_threat) weights.push_back(w);

		// bishop pin
		for (auto w{0}; w < num_ranks; ++w) weights.push_back(evaluate::bishop_pin[white][w]);
		for (auto w{0}; w < num_ranks; ++w) weights.push_back(evaluate::bishop_pin[black][w]);

		// mobility tables
		for (auto& w : evaluate::mob_mult_const) weights.push_back(w);
		for (auto& w : evaluate::mob_mult_rank_quad) weights.push_back(w);
		for (auto& w : evaluate::mob_mult_file_quad) weights.push_back(w);
		for (auto& w : evaluate::mob_mult_center) weights.push_back(w);
		for (auto& w : evaluate::mob_mult_rank) weights.push_back(w);
		for (auto& w : evaluate::mob_mult_edge) weights.push_back(w);

		// king danger
		for (auto& w : evaluate::king_danger) weights.push_back(w);

		// eval_pawns
		weights.push_back(pawn::center_bind);
		weights.push_back(pawn::multiple_passed_pawns);
		weights.push_back(pawn::second_row_fixed);

		// init
		weights.push_back(pawn::max_safety_bonus);
		weights.push_back(pawn::pawn_unsupported);
		weights.push_back(pawn::chain_mult);
		weights.push_back(pawn::chain_div);
		weights.push_back(pawn::file_factor_mult);

		// eval_shelter_storm
		weights.push_back(pawn::ss_base);
		weights.push_back(pawn::ss_safety_factor);
		weights.push_back(pawn::ss_danger_factor);

		// calculate_king_safety
		weights.push_back(pawn::safe_bonus_div);
		weights.push_back(pawn::safe_bonus_mult);
		weights.push_back(pawn::safe_bonus_mult_r34);
		weights.push_back(pawn::safe_bonus_mult_r5);
		weights.push_back(pawn::king_1st_rank);
		weights.push_back(pawn::king_near_enemy_pawns);

		// ps factors
		weights.push_back(pawn::mg_mg_mult);
		weights.push_back(pawn::mg_eg_mult);
		weights.push_back(pawn::eg_mg_mult);
		weights.push_back(pawn::eg_eg_mult);
		weights.push_back(pawn::ps_div);

		//pawn shield/storm
		for (auto& w : pawn::shield_factor) weights.push_back(w);
		for (auto& w : pawn::storm_factor) weights.push_back(w);
		for (auto& w : pawn::pawn_shield_constants) weights.push_back(w);
		for (auto& w : pawn::pawn_storm_constants) weights.push_back(w);

		//pawn phalanx
		for (auto& w : pawn::phalanx_seed) weights.push_back(w);
		for (auto& w : pawn::seed) weights.push_back(w);
		for (auto& w : pawn::remaining_score) weights.push_back(w);
		for (auto& w : pawn::un_supported_pawn) weights.push_back(w);
		for (auto& w : pawn::pawn_attacker_score) weights.push_back(w);

		// passed pawn
		for (auto& w : pawn::passed_pawn_values) weights.push_back(w);
		for (auto& w : pawn::passed_pawn_values_2) weights.push_back(w);

		// doubled/isolated pawn
		for (auto& w : pawn::doubled_pawn) weights.push_back(w);
		for (auto w{0}; w < num_ranks; ++w) weights.push_back(pawn::isolated_pawn[white][w]);
		for (auto w{0}; w < num_ranks; ++w) weights.push_back(pawn::isolated_pawn[black][w]);

		// pawn tables
		for (auto w{0}; w < num_ranks; ++w) weights.push_back(pawn::shelter_weakness[0][w]);
		for (auto w{0}; w < num_ranks; ++w) weights.push_back(pawn::shelter_weakness[1][w]);
		for (auto w{0}; w < num_ranks; ++w) weights.push_back(pawn::shelter_weakness[2][w]);
		for (auto w{0}; w < num_ranks; ++w) weights.push_back(pawn::shelter_weakness[3][w]);

		for (auto w{0}; w < num_ranks; ++w) weights.push_back(pawn::storm_danger[0][0][w]);
		for (auto w{0}; w < num_ranks; ++w) weights.push_back(pawn::storm_danger[0][1][w]);
		for (auto w{0}; w < num_ranks; ++w) weights.push_back(pawn::storm_danger[0][2][w]);
		for (auto w{0}; w < num_ranks; ++w) weights.push_back(pawn::storm_danger[0][3][w]);

		// material
		// pawn factors
		weights.push_back(material::p_base_score);
		weights.push_back(material::p_q_factor);
		weights.push_back(material::p_r_factor);
		weights.push_back(material::p_b_factor);
		weights.push_back(material::p_n_factor);

		// knight factors
		weights.push_back(material::n_base_score);
		weights.push_back(material::n_q_factor);
		weights.push_back(material::n_r_factor);
		weights.push_back(material::n_b_factor);
		weights.push_back(material::n_n_factor);
		weights.push_back(material::n_p_factor);

		// bishop factors
		weights.push_back(material::b_base_score);
		weights.push_back(material::b_q_factor);
		weights.push_back(material::b_r_factor);
		weights.push_back(material::b_b_factor);
		weights.push_back(material::b_n_factor);
		weights.push_back(material::b_p_factor);

		// rook factors
		weights.push_back(material::r_base_score);
		weights.push_back(material::r_q_factor);
		weights.push_back(material::r_r_factor);
		weights.push_back(material::r_b_factor);
		weights.push_back(material::r_n_factor);
		weights.push_back(material::r_p_factor);

		// queen factors
		weights.push_back(material::q_base_score);
		weights.push_back(material::q_q_factor);
		weights.push_back(material::q_r_factor);
		weights.push_back(material::q_b_factor);
		weights.push_back(material::q_n_factor);
		weights.push_back(material::q_p_factor);

		// bishop pair factors
		weights.push_back(material::bp_base_score);
		weights.push_back(material::bp_q_factor);
		weights.push_back(material::bp_r_factor);
		weights.push_back(material::bp_b_factor);
		weights.push_back(material::bp_n_factor);

		// material imbalance
		weights.push_back(material::up_two_pieces_bonus);
		weights.push_back(material::more_bishops_bonus);
		weights.push_back(material::more_knights_bonus);

		// phase factors
		weights.push_back(material::max_phase);
		weights.push_back(material::r_phase_factor);
		weights.push_back(material::q_phase_factor);

// material imbalance
	}

	void save_weights(const std::vector<int>& weights)
	{
		// copying the tuned evaluation weights back to the evaluation
		int c{};

		// eval
		// init mobility mult
		evaluate::mob_factor_p = weights[c++];
		evaluate::mmrq_factor_p = weights[c++];
		evaluate::mmfq_factor_p = weights[c++];
		evaluate::mmc_factor_p = weights[c++];
		evaluate::mmr_factor_p = weights[c++];
		evaluate::mme_factor_p = weights[c++];

		evaluate::mob_factor_b1 = weights[c++];
		evaluate::mob_factor_b2 = weights[c++];
		evaluate::mmrq_factor_b2 = weights[c++];
		evaluate::mmfq_factor_b2 = weights[c++];
		evaluate::mmc_factor_b2 = weights[c++];
		evaluate::mmr_factor_b2 = weights[c++];
		evaluate::mme_factor_b2 = weights[c++];

		evaluate::mob_factor_r = weights[c++];
		evaluate::mmrq_factor_r = weights[c++];
		evaluate::mmfq_factor_r = weights[c++];
		evaluate::mmc_factor_r = weights[c++];
		evaluate::mmr_factor_r = weights[c++];
		evaluate::mme_factor_r = weights[c++];

		evaluate::mob_factor_q = weights[c++];
		evaluate::mmrq_factor_q = weights[c++];
		evaluate::mmfq_factor_q = weights[c++];
		evaluate::mmc_factor_q = weights[c++];
		evaluate::mmr_factor_q = weights[c++];
		evaluate::mme_factor_q = weights[c++];

		// init pawn mobility values
		evaluate::pawn_mg_mult = weights[c++];
		evaluate::pawn_mg_sub = weights[c++];
		evaluate::pawn_eg_mult = weights[c++];
		evaluate::pawn_eg_sub = weights[c++];

		// init bishop mobility values
		evaluate::b1_mg_mult = weights[c++];
		evaluate::b1_mg_sub = weights[c++];
		evaluate::b1_eg_mult = weights[c++];
		evaluate::b1_eg_sub = weights[c++];

		evaluate::b2_mg_mult = weights[c++];
		evaluate::b2_mg_sub = weights[c++];
		evaluate::b2_eg_mult = weights[c++];
		evaluate::b2_eg_sub = weights[c++];

		// init rook mobility values
		evaluate::rook_mg_mult = weights[c++];
		evaluate::rook_mg_sub = weights[c++];
		evaluate::rook_eg_mult = weights[c++];
		evaluate::rook_eg_sub = weights[c++];
		evaluate::mob_r_mult = weights[c++];
		evaluate::mob_r_div = weights[c++];

		// init queen mobility values
		evaluate::queen_mg_mult = weights[c++];
		evaluate::queen_mg_sub = weights[c++];
		evaluate::queen_eg_mult = weights[c++];
		evaluate::queen_eg_sub = weights[c++];

		// init distance values
		evaluate::p_k_distance = weights[c++];
		evaluate::p_k_distance_mult = weights[c++];
		evaluate::b_k_distance = weights[c++];
		evaluate::b_k_distance_mult = weights[c++];

		// init pawn-bishop color values
		evaluate::pawn_on_bishop_color = weights[c++];
		evaluate::pawn_on_other_bishop_color = weights[c++];
		evaluate::pawn_file_width_mg = weights[c++];
		evaluate::pawn_file_width_eg = weights[c++];
		evaluate::threats_score = weights[c++];

		// init passed pawn arrays
		evaluate::pp_dvd_mgfactor = weights[c++];
		evaluate::pp_dvd_egfactor = weights[c++];
		evaluate::pp_ndvd_mgfactor = weights[c++];
		evaluate::pp_ndvd_egfactor = weights[c++];

		evaluate::pp_fp_base_mg = weights[c++];
		evaluate::pp_fp_base_eg = weights[c++];
		evaluate::pp_fp_mg = weights[c++];
		evaluate::pp_fp_eg = weights[c++];
		evaluate::pp_fp_mul = weights[c++];
		evaluate::pp_fp_div = weights[c++];

		evaluate::pp_as_base_mg = weights[c++];
		evaluate::pp_as_base_eg = weights[c++];
		evaluate::pp_as_mg = weights[c++];
		evaluate::pp_as_eg = weights[c++];
		evaluate::pp_as_mul = weights[c++];
		evaluate::pp_as_div = weights[c++];

		evaluate::pp_ab_base_mg = weights[c++];
		evaluate::pp_ab_base_eg = weights[c++];

		evaluate::pp_support_proximity_factor = weights[c++];
		evaluate::pp_mk_kdfp_factor = weights[c++];
		evaluate::pp_yk_kdfp_factor = weights[c++];
		evaluate::pp_mk_factor = weights[c++];
		evaluate::pp_mk_div = weights[c++];
		evaluate::pp_yk_factor = weights[c++];
		evaluate::pp_yk_div = weights[c++];

		// calculate_scale_factor
		evaluate::sf_mult = weights[c++];
		evaluate::sf_div = weights[c++];

		// eval_bishops
		evaluate::bishop_in_front_of_king = weights[c++];
		evaluate::bishop_in_corner = weights[c++];
		evaluate::trapped_bishop_extra = weights[c++];
		evaluate::trapped_bishop = weights[c++];
		evaluate::bishop_dominates_pawn = weights[c++];
		evaluate::k_zone_attack_bonus = weights[c++];

		// eval_initiative
		evaluate::initiative_mult = weights[c++];

		// eval_king_attack
		evaluate::k_attack_index_factor = weights[c++];
		evaluate::k_attack_pin_factor = weights[c++];
		evaluate::k_attack_sd_factor = weights[c++];
		evaluate::cspan_safe = weights[c++];
		evaluate::cspan = weights[c++];
		evaluate::csbab_safe = weights[c++];
		evaluate::csbab_safe = weights[c++];
		evaluate::csrar_safe = weights[c++];
		evaluate::csrar = weights[c++];
		evaluate::qcayk_all = weights[c++];
		evaluate::qcayk = weights[c++];
		evaluate::queen_check_bonus = weights[c++];

		// eval_knights
		evaluate::knight_attack_king = weights[c++];
		evaluate::p_mobility_add = weights[c++];
		evaluate::p_mobility_div = weights[c++];

		// eval_passed_pawns
		evaluate::passed_pawn_mk_mult = weights[c++];
		evaluate::passed_pawn_mk_div = weights[c++];
		evaluate::passed_pawn_yk_mult = weights[c++];
		evaluate::passed_pawn_yk_div = weights[c++];
		evaluate::passed_pawn_mk_md_mul = weights[c++];
		evaluate::passed_pawn_mk_md_div = weights[c++];
		evaluate::passed_pawn_yk_md_mul = weights[c++];
		evaluate::passed_pawn_yk_md_div = weights[c++];
		evaluate::bb_behind_passed_pawn_bonus = weights[c++];

		// eval_queens
		evaluate::queen_attack_king = weights[c++];
		evaluate::queen_attack_king_zone = weights[c++];
		evaluate::r_mobility_add = weights[c++];
		evaluate::r_mobility_div = weights[c++];

		// eval_rooks
		evaluate::uncastled_penalty = weights[c++];
		evaluate::rook_attacks_king = weights[c++];
		evaluate::rook_traps_king_on_7th = weights[c++];
		evaluate::no_pawn = weights[c++];
		evaluate::pawn_attacks = weights[c++];
		evaluate::pawn_no_attack = weights[c++];
		evaluate::q_mobility_add = weights[c++];
		evaluate::q_mobility_div = weights[c++];

		// eval_space
		evaluate::space_threshold = weights[c++];
		evaluate::space_weight_mult = weights[c++];
		evaluate::space_weight_div = weights[c++];

		// eval_strong_squares
		evaluate::safety_for_pawn_rbp = weights[c++];
		evaluate::strong_p_in_front_of_pawn = weights[c++];
		evaluate::strong_square_pb = weights[c++];
		evaluate::strong_square_pb_extra = weights[c++];
		evaluate::pb_behind_pawn = weights[c++];
		evaluate::protected_piece = weights[c++];

		// eval_threats
		evaluate::hanging_pawn_threat = weights[c++];
		evaluate::hanging_pieces = weights[c++];
		evaluate::king_threat_single = weights[c++];
		evaluate::king_threat_multiple = weights[c++];
		evaluate::pawn_advance = weights[c++];

		// eval
		evaluate::blocked_pawns_mg = weights[c++];
		evaluate::blocked_pawns_eg = weights[c++];
		evaluate::mg_mgvalue_mult = weights[c++];
		evaluate::mg_egvalue_mult = weights[c++];
		evaluate::eg_mgvalue_mult = weights[c++];
		evaluate::eg_egvalue_mult = weights[c++];
		evaluate::eval_mult = weights[c++];
		evaluate::conversion_mult = weights[c++];
		evaluate::conversion_div = weights[c++];
		evaluate::eval_div = weights[c++];
		evaluate::eval_value_div = weights[c++];
		evaluate::flank_double_attack = weights[c++];
		evaluate::pawn_contempt_mult = weights[c++];
		evaluate::knight_contempt_mult = weights[c++];
		evaluate::bishop_contempt_mult = weights[c++];
		evaluate::rook_contempt_mult = weights[c++];
		evaluate::queen_contempt_mult = weights[c++];
		evaluate::contempt_mult = weights[c++];

		for (auto& w : evaluate::passed_pawn_proximity) w = weights[c++];

		// threat arrays
		for (auto& w : evaluate::piece_threat) w = weights[c++];
		for (auto& w : evaluate::rook_threat) w = weights[c++];
		for (auto& w : evaluate::pawn_threat) w = weights[c++];

		// bishop pin
		for (auto w{0}; w < num_ranks; ++w) evaluate::bishop_pin[white][w] = weights[c++];
		for (auto w{0}; w < num_ranks; ++w) evaluate::bishop_pin[black][w] = weights[c++];

		// mobility tables
		for (auto& w : evaluate::mob_mult_const) w = weights[c++];
		for (auto& w : evaluate::mob_mult_rank_quad) w = weights[c++];
		for (auto& w : evaluate::mob_mult_file_quad) w = weights[c++];
		for (auto& w : evaluate::mob_mult_center) w = weights[c++];
		for (auto& w : evaluate::mob_mult_rank) w = weights[c++];
		for (auto& w : evaluate::mob_mult_edge) w = weights[c++];

		// king danger	
		for (auto& w : evaluate::king_danger) w = weights[c++];

		// 	eval_pawns
		pawn::center_bind = weights[c++];
		pawn::multiple_passed_pawns = weights[c++];
		pawn::second_row_fixed = weights[c++];

		// init
		pawn::max_safety_bonus = weights[c++];
		pawn::pawn_unsupported = weights[c++];
		pawn::chain_mult = weights[c++];
		pawn::chain_div = weights[c++];
		pawn::file_factor_mult = weights[c++];

		// eval_shelter_storm
		pawn::ss_base = weights[c++];
		pawn::ss_safety_factor = weights[c++];
		pawn::ss_danger_factor = weights[c++];

		// calculate_king_safety
		pawn::safe_bonus_div = weights[c++];
		pawn::safe_bonus_mult = weights[c++];
		pawn::safe_bonus_mult_r34 = weights[c++];
		pawn::safe_bonus_mult_r5 = weights[c++];
		pawn::king_1st_rank = static_cast<int>(weights[c++]);
		pawn::king_near_enemy_pawns = static_cast<int>(weights[c++]);

		// ps factors
		pawn::mg_mg_mult = weights[c++];
		pawn::mg_eg_mult = weights[c++];
		pawn::eg_mg_mult = weights[c++];
		pawn::eg_eg_mult = weights[c++];
		pawn::ps_div = weights[c++];

		//pawn shield/storm
		for (auto& w : pawn::shield_factor) w = weights[c++];
		for (auto& w : pawn::storm_factor) w = weights[c++];
		for (auto& w : pawn::pawn_shield_constants) w = weights[c++];
		for (auto& w : pawn::pawn_storm_constants) w = weights[c++];

		//pawn phalanx
		for (auto& w : pawn::phalanx_seed) w = weights[c++];
		for (auto& w : pawn::seed) w = weights[c++];
		for (auto& w : pawn::remaining_score) w = weights[c++];
		for (auto& w : pawn::un_supported_pawn) w = weights[c++];
		for (auto& w : pawn::pawn_attacker_score) w = weights[c++];

		// passed pawn
		for (auto& w : pawn::passed_pawn_values) w = weights[c++];
		for (auto& w : pawn::passed_pawn_values_2) w = weights[c++];

		// doubled/isolated pawn
		for (auto& w : pawn::doubled_pawn) w = static_cast<score>(weights[c++]);
		for (auto w{0}; w < num_files; ++w) pawn::isolated_pawn[white][w] = static_cast<int>(weights[c++]);
		for (auto w{0}; w < num_files; ++w) pawn::isolated_pawn[black][w] = static_cast<int>(weights[c++]);

		// pawn tables
		for (auto w{0}; w < num_ranks; ++w) pawn::shelter_weakness[0][w] = static_cast<int>(weights[c++]);
		for (auto w{0}; w < num_ranks; ++w) pawn::shelter_weakness[1][w] = static_cast<int>(weights[c++]);
		for (auto w{0}; w < num_ranks; ++w) pawn::shelter_weakness[2][w] = static_cast<int>(weights[c++]);
		for (auto w{0}; w < num_ranks; ++w) pawn::shelter_weakness[3][w] = static_cast<int>(weights[c++]);

		for (auto w{0}; w < num_ranks; ++w) pawn::storm_danger[0][0][w] = static_cast<int>(weights[c++]);
		for (auto w{0}; w < num_ranks; ++w) pawn::storm_danger[0][1][w] = static_cast<int>(weights[c++]);
		for (auto w{0}; w < num_ranks; ++w) pawn::storm_danger[0][2][w] = static_cast<int>(weights[c++]);
		for (auto w{0}; w < num_ranks; ++w) pawn::storm_danger[0][3][w] = static_cast<int>(weights[c++]);

		// material
		// pawn factors
		material::p_base_score = weights[c++];
		material::p_q_factor = weights[c++];
		material::p_r_factor = weights[c++];
		material::p_b_factor = weights[c++];
		material::p_n_factor = weights[c++];

		// knight factors
		material::n_base_score = weights[c++];
		material::n_q_factor = weights[c++];
		material::n_r_factor = weights[c++];
		material::n_b_factor = weights[c++];
		material::n_n_factor = weights[c++];
		material::n_p_factor = weights[c++];

		// bishop factors
		material::b_base_score = weights[c++];
		material::b_q_factor = weights[c++];
		material::b_r_factor = weights[c++];
		material::b_b_factor = weights[c++];
		material::b_n_factor = weights[c++];
		material::b_p_factor = weights[c++];

		// rook factors
		material::r_base_score = weights[c++];
		material::r_q_factor = weights[c++];
		material::r_r_factor = weights[c++];
		material::r_b_factor = weights[c++];
		material::r_n_factor = weights[c++];
		material::r_p_factor = weights[c++];

		// queen factors
		material::q_base_score = weights[c++];
		material::q_q_factor = weights[c++];
		material::q_r_factor = weights[c++];
		material::q_b_factor = weights[c++];
		material::q_n_factor = weights[c++];
		material::q_p_factor = weights[c++];

		// bishop pair factors
		material::bp_base_score = weights[c++];
		material::bp_q_factor = weights[c++];
		material::bp_r_factor = weights[c++];
		material::bp_b_factor = weights[c++];
		material::bp_n_factor = weights[c++];

		// material imbalance
		material::up_two_pieces_bonus = weights[c++];
		material::more_bishops_bonus = weights[c++];
		material::more_knights_bonus = weights[c++];

		// phase factors
		material::max_phase = weights[c++];
		material::r_phase_factor = weights[c++];
		material::q_phase_factor = weights[c++];
	}

	void display_weights()
	{
		// displaying weights and write to disk file

		static char file_name[256]{};
		char buf[256]{};

		// calculate time stamp for file name
		auto now = time(nullptr);
		strftime(buf, 32, "%b-%d_%H-%M", localtime(&now));

		// copy time stamp string to file name
		sprintf(file_name, "tune_%s.txt", buf);

		std::ofstream tune_log;
		tune_log.open(file_name);
		tune_log << program << " " << version << " " << platform << " " << bmis << std::endl;
		tune_log << std::endl;

		// evaluation
		// init mobility mult
		acout() << "init mobility mult" << std::endl;
		acout() << "mob_factor_p: " << evaluate::mob_factor_p << std::endl;
		acout() << "mmrq_factor_p: " << evaluate::mmrq_factor_p << std::endl;
		acout() << "mmfq_factor_p: " << evaluate::mmfq_factor_p << std::endl;
		acout() << "mmc_factor_p: " << evaluate::mmc_factor_p << std::endl;
		acout() << "mmr_factor_p: " << evaluate::mmr_factor_p << std::endl;
		acout() << "mme_factor_p: " << evaluate::mme_factor_p << std::endl;

		acout() << "mob_factor_b1: " << evaluate::mob_factor_b1 << std::endl;
		acout() << "mob_factor_b2: " << evaluate::mob_factor_b2 << std::endl;
		acout() << "mmrq_factor_b2: " << evaluate::mmrq_factor_b2 << std::endl;
		acout() << "mmfq_factor_b2: " << evaluate::mmfq_factor_b2 << std::endl;
		acout() << "mmc_factor_b2: " << evaluate::mmc_factor_b2 << std::endl;
		acout() << "mmr_factor_b2: " << evaluate::mmr_factor_b2 << std::endl;
		acout() << "mme_factor_b2: " << evaluate::mmr_factor_b2 << std::endl;

		acout() << "mob_factor_r: " << evaluate::mob_factor_r << std::endl;
		acout() << "mmrq_factor_r: " << evaluate::mmrq_factor_r << std::endl;
		acout() << "mmfq_factor_r: " << evaluate::mmfq_factor_r << std::endl;
		acout() << "mmc_factor_r: " << evaluate::mmc_factor_r << std::endl;
		acout() << "mmr_factor_r: " << evaluate::mmr_factor_r << std::endl;
		acout() << "mme_factor_r: " << evaluate::mme_factor_r << std::endl;

		acout() << "mob_factor_q: " << evaluate::mob_factor_q << std::endl;
		acout() << "mmrq_factor_q: " << evaluate::mmrq_factor_q << std::endl;
		acout() << "mmfq_factor_q: " << evaluate::mmfq_factor_q << std::endl;
		acout() << "mmc_factor_q: " << evaluate::mmc_factor_q << std::endl;
		acout() << "mmr_factor_q: " << evaluate::mmr_factor_q << std::endl;
		acout() << "mme_factor_q: " << evaluate::mme_factor_q << std::endl;
		acout() << std::endl;

		tune_log << "init mobility mult" << std::endl;
		tune_log << "mob_factor_p: " << evaluate::mob_factor_p << std::endl;
		tune_log << "mmrq_factor_p: " << evaluate::mmrq_factor_p << std::endl;
		tune_log << "mmfq_factor_p: " << evaluate::mmfq_factor_p << std::endl;
		tune_log << "mmc_factor_p: " << evaluate::mmc_factor_p << std::endl;
		tune_log << "mmr_factor_p: " << evaluate::mmr_factor_p << std::endl;
		tune_log << "mme_factor_p: " << evaluate::mme_factor_p << std::endl;

		tune_log << "mob_factor_b1: " << evaluate::mob_factor_b1 << std::endl;
		tune_log << "mob_factor_b2: " << evaluate::mob_factor_b2 << std::endl;
		tune_log << "mmrq_factor_b2: " << evaluate::mmrq_factor_b2 << std::endl;
		tune_log << "mmfq_factor_b2: " << evaluate::mmfq_factor_b2 << std::endl;
		tune_log << "mmc_factor_b2: " << evaluate::mmc_factor_b2 << std::endl;
		tune_log << "mmr_factor_b2: " << evaluate::mmr_factor_b2 << std::endl;
		tune_log << "mme_factor_b2: " << evaluate::mmr_factor_b2 << std::endl;

		tune_log << "mob_factor_r: " << evaluate::mob_factor_r << std::endl;
		tune_log << "mmrq_factor_r: " << evaluate::mmrq_factor_r << std::endl;
		tune_log << "mmfq_factor_r: " << evaluate::mmfq_factor_r << std::endl;
		tune_log << "mmc_factor_r: " << evaluate::mmc_factor_r << std::endl;
		tune_log << "mmr_factor_r: " << evaluate::mmr_factor_r << std::endl;
		tune_log << "mme_factor_r: " << evaluate::mme_factor_r << std::endl;

		tune_log << "mob_factor_q: " << evaluate::mob_factor_q << std::endl;
		tune_log << "mmrq_factor_q: " << evaluate::mmrq_factor_q << std::endl;
		tune_log << "mmfq_factor_q: " << evaluate::mmfq_factor_q << std::endl;
		tune_log << "mmc_factor_q: " << evaluate::mmc_factor_q << std::endl;
		tune_log << "mmr_factor_q: " << evaluate::mmr_factor_q << std::endl;
		tune_log << "mme_factor_q: " << evaluate::mme_factor_q << std::endl;
		tune_log << std::endl;

		// init pawn mobility values
		acout() << "pawn_mg_mult: " << evaluate::pawn_mg_mult << std::endl;
		acout() << "pawn_mg_sub: " << evaluate::pawn_mg_sub << std::endl;
		acout() << "pawn_eg_mult: " << evaluate::pawn_eg_mult << std::endl;
		acout() << "pawn_eg_sub: " << evaluate::pawn_eg_sub << std::endl;
		acout() << std::endl;

		tune_log << "pawn_mg_mult: " << evaluate::pawn_mg_mult << std::endl;
		tune_log << "pawn_mg_sub: " << evaluate::pawn_mg_sub << std::endl;
		tune_log << "pawn_eg_mult: " << evaluate::pawn_eg_mult << std::endl;
		tune_log << "pawn_eg_sub: " << evaluate::pawn_eg_sub << std::endl;
		tune_log << std::endl;

		// init bishop mobility values
		acout() << "init bishop mobility values" << std::endl;
		acout() << "b1_mg_mult: " << evaluate::b1_mg_mult << std::endl;
		acout() << "b1_mg_sub: " << evaluate::b1_mg_sub << std::endl;
		acout() << "b1_eg_mult: " << evaluate::b1_eg_mult << std::endl;
		acout() << "b1_eg_sub: " << evaluate::b1_eg_sub << std::endl;

		acout() << "b2_mg_mult: " << evaluate::b2_mg_mult << std::endl;
		acout() << "b2_mg_sub: " << evaluate::b2_mg_sub << std::endl;
		acout() << "b2_eg_mult: " << evaluate::b2_eg_mult << std::endl;
		acout() << "b2_eg_sub: " << evaluate::b2_eg_sub << std::endl;
		acout() << std::endl;

		tune_log << "init bishop mobility values" << std::endl;
		tune_log << "b1_mg_mult: " << evaluate::b1_mg_mult << std::endl;
		tune_log << "b1_mg_sub: " << evaluate::b1_mg_sub << std::endl;
		tune_log << "b1_eg_mult: " << evaluate::b1_eg_mult << std::endl;
		tune_log << "b1_eg_sub: " << evaluate::b1_eg_sub << std::endl;

		tune_log << "b2_mg_mult: " << evaluate::b2_mg_mult << std::endl;
		tune_log << "b2_mg_sub: " << evaluate::b2_mg_sub << std::endl;
		tune_log << "b2_eg_mult: " << evaluate::b2_eg_mult << std::endl;
		tune_log << "b2_eg_sub: " << evaluate::b2_eg_sub << std::endl;
		tune_log << std::endl;

		// init rook mobility values
		acout() << "init rook mobility values" << std::endl;
		acout() << "rook_mg_mult: " << evaluate::rook_mg_mult << std::endl;
		acout() << "rook_mg_sub: " << evaluate::rook_mg_sub << std::endl;
		acout() << "rook_eg_mult: " << evaluate::rook_eg_mult << std::endl;
		acout() << "rook_eg_sub: " << evaluate::rook_eg_sub << std::endl;
		acout() << "mob_r_mult: " << evaluate::mob_r_mult << std::endl;
		acout() << "mob_r_div: " << evaluate::mob_r_div << std::endl;
		acout() << std::endl;

		tune_log << "init rook mobility values" << std::endl;
		tune_log << "rook_mg_mult: " << evaluate::rook_mg_mult << std::endl;
		tune_log << "rook_mg_sub: " << evaluate::rook_mg_sub << std::endl;
		tune_log << "rook_eg_mult: " << evaluate::rook_eg_mult << std::endl;
		tune_log << "rook_eg_sub: " << evaluate::rook_eg_sub << std::endl;
		tune_log << "mob_r_mult: " << evaluate::mob_r_mult << std::endl;
		tune_log << "mob_r_div: " << evaluate::mob_r_div << std::endl;
		tune_log << std::endl;

		// init queen mobility values
		acout() << "init queen mobility values" << std::endl;
		acout() << "queen_mg_mult: " << evaluate::queen_mg_mult << std::endl;
		acout() << "queen_mg_sub: " << evaluate::queen_mg_sub << std::endl;
		acout() << "queen_eg_mult: " << evaluate::queen_eg_mult << std::endl;
		acout() << "queen_eg_sub: " << evaluate::queen_eg_sub << std::endl;
		acout() << std::endl;

		tune_log << "init queen mobility values" << std::endl;
		tune_log << "queen_mg_mult: " << evaluate::queen_mg_mult << std::endl;
		tune_log << "queen_mg_sub: " << evaluate::queen_mg_sub << std::endl;
		tune_log << "queen_eg_mult: " << evaluate::queen_eg_mult << std::endl;
		tune_log << "queen_eg_sub: " << evaluate::queen_eg_sub << std::endl;
		tune_log << std::endl;

		// init distance values
		acout() << "init distance values" << std::endl;
		acout() << "p_k_distance: " << evaluate::p_k_distance << std::endl;
		acout() << "p_k_distance_mult: " << evaluate::p_k_distance_mult << std::endl;
		acout() << "b_k_distance: " << evaluate::b_k_distance << std::endl;
		acout() << "b_k_distance_mult: " << evaluate::b_k_distance_mult << std::endl;
		acout() << std::endl;

		tune_log << "distance" << std::endl;
		tune_log << "p_k_distance: " << evaluate::p_k_distance << std::endl;
		tune_log << "p_k_distance_mult: " << evaluate::p_k_distance_mult << std::endl;
		tune_log << "b_k_distance: " << evaluate::b_k_distance << std::endl;
		tune_log << "b_k_distance_mult: " << evaluate::b_k_distance_mult << std::endl;
		tune_log << std::endl;

		// init pawn-bishop color values
		acout() << "pawn" << std::endl;
		acout() << "pawn_on_bishop_color: " << evaluate::pawn_on_bishop_color << std::endl;
		acout() << "pawn_on_other_bishop_color: " << evaluate::pawn_on_other_bishop_color << std::endl;
		acout() << "pawn_file_width_mg: " << evaluate::pawn_file_width_mg << std::endl;
		acout() << "pawn_file_width_eg: " << evaluate::pawn_file_width_eg << std::endl;
		acout() << "threats_score: " << evaluate::threats_score << std::endl;

		tune_log << "pawn" << std::endl;
		tune_log << "pawn_on_bishop_color: " << evaluate::pawn_on_bishop_color << std::endl;
		tune_log << "pawn_on_other_bishop_color: " << evaluate::pawn_on_other_bishop_color << std::endl;
		tune_log << "pawn_file_width_mg: " << evaluate::pawn_file_width_mg << std::endl;
		tune_log << "pawn_file_width_eg: " << evaluate::pawn_file_width_eg << std::endl;
		tune_log << "threats_score: " << evaluate::threats_score << std::endl;

		// init passed pawn arrays
		acout() << "init passed pawn arrays" << std::endl;
		acout() << "pp_dvd_mgfactor: " << evaluate::pp_dvd_mgfactor << std::endl;
		acout() << "pp_dvd_egfactor: " << evaluate::pp_dvd_egfactor << std::endl;
		acout() << "pp_ndvd_mgfactor: " << evaluate::pp_ndvd_mgfactor << std::endl;
		acout() << "pp_ndvd_egfactor: " << evaluate::pp_ndvd_egfactor << std::endl;

		acout() << "pp_fp_base_mg: " << evaluate::pp_fp_base_mg << std::endl;
		acout() << "pp_fp_base_eg: " << evaluate::pp_fp_base_eg << std::endl;
		acout() << "pp_fp_mg: " << evaluate::pp_fp_mg << std::endl;
		acout() << "pp_fp_eg: " << evaluate::pp_fp_eg << std::endl;
		acout() << "pp_fp_mul: " << evaluate::pp_fp_mul << std::endl;
		acout() << "pp_fp_div: " << evaluate::pp_fp_div << std::endl;

		acout() << "pp_as_base_mg: " << evaluate::pp_as_base_mg << std::endl;
		acout() << "pp_as_base_eg: " << evaluate::pp_as_base_eg << std::endl;
		acout() << "pp_as_mg: " << evaluate::pp_as_mg << std::endl;
		acout() << "pp_as_eg: " << evaluate::pp_as_eg << std::endl;
		acout() << "pp_as_mul: " << evaluate::pp_as_mul << std::endl;
		acout() << "pp_as_div: " << evaluate::pp_as_div << std::endl;

		acout() << "pp_ab_base_mg: " << evaluate::pp_ab_base_mg << std::endl;
		acout() << "pp_ab_base_eg: " << evaluate::pp_ab_base_eg << std::endl;

		acout() << "passed_pawn_proximity: {"; for (auto& w : evaluate::passed_pawn_proximity) acout() << w << ","; acout() << "}" << std::endl;
		acout() << "pp_support_proximity_factor: " << evaluate::pp_support_proximity_factor << std::endl;
		acout() << "pp_mk_kdfp_factor: " << evaluate::pp_mk_kdfp_factor << std::endl;
		acout() << "pp_yk_kdfp_factor: " << evaluate::pp_yk_kdfp_factor << std::endl;
		acout() << "pp_mk_factor: " << evaluate::pp_mk_factor << std::endl;
		acout() << "pp_mk_div: " << evaluate::pp_mk_div << std::endl;
		acout() << "pp_yk_factor: " << evaluate::pp_yk_factor << std::endl;
		acout() << "pp_yk_div: " << evaluate::pp_yk_div << std::endl;
		acout() << std::endl;

		tune_log << "init passed pawn arrays" << std::endl;
		tune_log << "pp_dvd_mgfactor: " << evaluate::pp_dvd_mgfactor << std::endl;
		tune_log << "pp_dvd_egfactor: " << evaluate::pp_dvd_egfactor << std::endl;
		tune_log << "pp_ndvd_mgfactor: " << evaluate::pp_ndvd_mgfactor << std::endl;
		tune_log << "pp_ndvd_egfactor: " << evaluate::pp_ndvd_egfactor << std::endl;

		tune_log << "pp_fp_base_mg: " << evaluate::pp_fp_base_mg << std::endl;
		tune_log << "pp_fp_base_eg: " << evaluate::pp_fp_base_eg << std::endl;
		tune_log << "pp_fp_mg: " << evaluate::pp_fp_mg << std::endl;
		tune_log << "pp_fp_eg: " << evaluate::pp_fp_eg << std::endl;
		tune_log << "pp_fp_mul: " << evaluate::pp_fp_mul << std::endl;
		tune_log << "pp_fp_div: " << evaluate::pp_fp_div << std::endl;

		tune_log << "pp_as_base_mg: " << evaluate::pp_as_base_mg << std::endl;
		tune_log << "pp_as_base_eg: " << evaluate::pp_as_base_eg << std::endl;
		tune_log << "pp_as_mg: " << evaluate::pp_as_mg << std::endl;
		tune_log << "pp_as_eg: " << evaluate::pp_as_eg << std::endl;
		tune_log << "pp_as_mul: " << evaluate::pp_as_mul << std::endl;
		tune_log << "pp_as_div: " << evaluate::pp_as_div << std::endl;

		tune_log << "pp_ab_base_mg: " << evaluate::pp_ab_base_mg << std::endl;
		tune_log << "pp_ab_base_eg: " << evaluate::pp_ab_base_eg << std::endl;

		tune_log << "passed_pawn_proximity: {"; for (auto& w : evaluate::passed_pawn_proximity) tune_log << w << ","; tune_log << "}" << std::endl;
		tune_log << "pp_support_proximity_factor: " << evaluate::pp_support_proximity_factor << std::endl;
		tune_log << "pp_mk_kdfp_factor: " << evaluate::pp_mk_kdfp_factor << std::endl;
		tune_log << "pp_yk_kdfp_factor: " << evaluate::pp_yk_kdfp_factor << std::endl;
		tune_log << "pp_mk_factor: " << evaluate::pp_mk_factor << std::endl;
		tune_log << "pp_mk_div: " << evaluate::pp_mk_div << std::endl;
		tune_log << "pp_yk_factor: " << evaluate::pp_yk_factor << std::endl;
		tune_log << "pp_yk_div: " << evaluate::pp_yk_div << std::endl;
		tune_log << std::endl;

		// calculate_scale_factor
		acout() << "calculate_scale_factor" << std::endl;
		acout() << "sf_mult: " << evaluate::sf_mult << std::endl;
		acout() << "sf_div: " << evaluate::sf_div << std::endl;
		acout() << std::endl;

		tune_log << "calculate_scale_factor" << std::endl;
		tune_log << "sf_mult: " << evaluate::sf_mult << std::endl;
		tune_log << "sf_div: " << evaluate::sf_div << std::endl;
		tune_log << std::endl;

		// eval_bishops
		acout() << "eval_bishops" << std::endl;
		acout() << "bishop_in_front_of_king: " << evaluate::bishop_in_front_of_king << std::endl;
		acout() << "bishop_in_corner: " << evaluate::bishop_in_corner << std::endl;
		acout() << "trapped_bishop_extra: " << evaluate::trapped_bishop_extra << std::endl;
		acout() << "trapped_bishop: " << evaluate::trapped_bishop << std::endl;
		acout() << "bishop_dominates_pawn: " << evaluate::bishop_dominates_pawn << std::endl;
		acout() << "k_zone_attack_bonus: " << evaluate::k_zone_attack_bonus << std::endl;
		acout() << std::endl;

		tune_log << "eval_bishops" << std::endl;
		tune_log << "bishop_in_front_of_king: " << evaluate::bishop_in_front_of_king << std::endl;
		tune_log << "bishop_in_corner: " << evaluate::bishop_in_corner << std::endl;
		tune_log << "trapped_bishop_extra: " << evaluate::trapped_bishop_extra << std::endl;
		tune_log << "trapped_bishop: " << evaluate::trapped_bishop << std::endl;
		tune_log << "bishop_dominates_pawn: " << evaluate::bishop_dominates_pawn << std::endl;
		tune_log << "k_zone_attack_bonus: " << evaluate::k_zone_attack_bonus << std::endl;
		tune_log << std::endl;

		// eval_initiative
		acout() << "eval_initiative" << std::endl;
		acout() << "initiative_mult: " << evaluate::initiative_mult << std::endl;
		acout() << std::endl;

		tune_log << "eval_initiative" << std::endl;
		tune_log << "initiative_mult: " << evaluate::initiative_mult << std::endl;
		tune_log << std::endl;

		// eval_king_attack
		acout() << "eval_king_attack" << std::endl;
		acout() << "k_attack_index_factor: " << evaluate::k_attack_index_factor << std::endl;
		acout() << "k_attack_pin_factor: " << evaluate::k_attack_pin_factor << std::endl;
		acout() << "k_attack_sd_factor: " << evaluate::k_attack_sd_factor << std::endl;
		acout() << "cspan_safe: " << evaluate::cspan_safe << std::endl;
		acout() << "cspan: " << evaluate::cspan << std::endl;
		acout() << "csbab_safe: " << evaluate::csbab_safe << std::endl;
		acout() << "csbab: " << evaluate::csbab << std::endl;
		acout() << "csrar_safe: " << evaluate::csrar_safe << std::endl;
		acout() << "csrar: " << evaluate::csrar << std::endl;
		acout() << "qcayk_all: " << evaluate::qcayk_all << std::endl;
		acout() << "qcayk: " << evaluate::qcayk << std::endl;
		acout() << "queen_check_bonus: " << evaluate::queen_check_bonus << std::endl;
		acout() << std::endl;

		tune_log << "eval_king_attack" << std::endl;
		tune_log << "k_attack_index_factor: " << evaluate::k_attack_index_factor << std::endl;
		tune_log << "k_attack_pin_factor: " << evaluate::k_attack_pin_factor << std::endl;
		tune_log << "k_attack_sd_factor: " << evaluate::k_attack_sd_factor << std::endl;
		tune_log << "cspan_safe: " << evaluate::cspan_safe << std::endl;
		tune_log << "cspan: " << evaluate::cspan << std::endl;
		tune_log << "csbab_safe: " << evaluate::csbab_safe << std::endl;
		tune_log << "csbab: " << evaluate::csbab << std::endl;
		tune_log << "csrar_safe: " << evaluate::csrar_safe << std::endl;
		tune_log << "csrar: " << evaluate::csrar << std::endl;
		tune_log << "qcayk_all: " << evaluate::qcayk_all << std::endl;
		tune_log << "qcayk: " << evaluate::qcayk << std::endl;
		tune_log << "queen_check_bonus: " << evaluate::queen_check_bonus << std::endl;
		tune_log << std::endl;

		// eval_knights
		acout() << "eval_knights" << std::endl;
		acout() << "knight_attack_king: " << evaluate::knight_attack_king << std::endl;
		acout() << "p_mobility_add: " << evaluate::p_mobility_add << std::endl;
		acout() << "p_mobility_div: " << evaluate::p_mobility_div << std::endl;
		acout() << std::endl;

		tune_log << "eval_knights" << std::endl;
		tune_log << "knight_attack_king: " << evaluate::knight_attack_king << std::endl;
		tune_log << "p_mobility_add: " << evaluate::p_mobility_add << std::endl;
		tune_log << "p_mobility_div: " << evaluate::p_mobility_div << std::endl;
		tune_log << std::endl;

		// eval_passed_pawns
		acout() << "eval_passed_pawns" << std::endl;
		acout() << "passed_pawn_mk_mult: " << evaluate::passed_pawn_mk_mult << std::endl;
		acout() << "passed_pawn_mk_div: " << evaluate::passed_pawn_mk_div << std::endl;
		acout() << "passed_pawn_yk_mult: " << evaluate::passed_pawn_yk_mult << std::endl;
		acout() << "passed_pawn_yk_div " << evaluate::passed_pawn_yk_div << std::endl;
		acout() << "passed_pawn_mk_md_mul: " << evaluate::passed_pawn_mk_md_mul << std::endl;
		acout() << "passed_pawn_mk_md_div: " << evaluate::passed_pawn_mk_md_div << std::endl;
		acout() << "passed_pawn_yk_md_mul: " << evaluate::passed_pawn_yk_md_mul << std::endl;
		acout() << "passed_pawn_yk_md_div: " << evaluate::passed_pawn_yk_md_div << std::endl;
		acout() << "bb_behind_passed_pawn_bonus: " << evaluate::bb_behind_passed_pawn_bonus << std::endl;
		acout() << std::endl;

		tune_log << "eval_passed_pawns" << std::endl;
		tune_log << "passed_pawn_mk_mult: " << evaluate::passed_pawn_mk_mult << std::endl;
		tune_log << "passed_pawn_mk_div: " << evaluate::passed_pawn_mk_div << std::endl;
		tune_log << "passed_pawn_yk_mult: " << evaluate::passed_pawn_yk_mult << std::endl;
		tune_log << "passed_pawn_yk_div " << evaluate::passed_pawn_yk_div << std::endl;
		tune_log << "passed_pawn_mk_md_mul: " << evaluate::passed_pawn_mk_md_mul << std::endl;
		tune_log << "passed_pawn_mk_md_div: " << evaluate::passed_pawn_mk_md_div << std::endl;
		tune_log << "passed_pawn_yk_md_mul: " << evaluate::passed_pawn_yk_md_mul << std::endl;
		tune_log << "passed_pawn_yk_md_div: " << evaluate::passed_pawn_yk_md_div << std::endl;
		tune_log << "bb_behind_passed_pawn_bonus: " << evaluate::bb_behind_passed_pawn_bonus << std::endl;
		tune_log << std::endl;

		// eval_queens
		acout() << "eval_queens" << std::endl;
		acout() << "queen_attack_king: " << evaluate::queen_attack_king << std::endl;
		acout() << "queen_attack_king_zone: " << evaluate::queen_attack_king_zone << std::endl;
		acout() << "q_mobility_add: " << evaluate::q_mobility_add << std::endl;
		acout() << "q_mobility_div: " << evaluate::q_mobility_div << std::endl;
		acout() << std::endl;

		tune_log << "eval_queens" << std::endl;
		tune_log << "queen_attack_king: " << evaluate::queen_attack_king << std::endl;
		tune_log << "queen_attack_king_zone: " << evaluate::queen_attack_king_zone << std::endl;
		tune_log << "q_mobility_add: " << evaluate::q_mobility_add << std::endl;
		tune_log << "q_mobility_div: " << evaluate::q_mobility_div << std::endl;
		tune_log << std::endl;

		// eval_rooks
		acout() << "eval_rooks" << std::endl;
		acout() << "uncastled_penalty: " << evaluate::uncastled_penalty << std::endl;
		acout() << "rook_attacks_king: " << evaluate::rook_attacks_king << std::endl;
		acout() << "rook_traps_king_on_7th: " << evaluate::rook_traps_king_on_7th << std::endl;
		acout() << "no_pawn: " << evaluate::no_pawn << std::endl;
		acout() << "pawn_attacks: " << evaluate::pawn_attacks << std::endl;
		acout() << "pawn_no_attack: " << evaluate::pawn_no_attack << std::endl;
		acout() << "r_mobility_add: " << evaluate::r_mobility_add << std::endl;
		acout() << "r_mobility_div: " << evaluate::r_mobility_div << std::endl;
		acout() << std::endl;

		tune_log << "eval_rooks" << std::endl;
		tune_log << "uncastled_penalty: " << evaluate::uncastled_penalty << std::endl;
		tune_log << "rook_attacks_king: " << evaluate::rook_attacks_king << std::endl;
		tune_log << "rook_traps_king_on_7th: " << evaluate::rook_traps_king_on_7th << std::endl;
		tune_log << "no_pawn: " << evaluate::no_pawn << std::endl;
		tune_log << "pawn_attacks: " << evaluate::pawn_attacks << std::endl;
		tune_log << "pawn_no_attack: " << evaluate::pawn_no_attack << std::endl;
		tune_log << "r_mobility_add: " << evaluate::r_mobility_add << std::endl;
		tune_log << "r_mobility_div: " << evaluate::r_mobility_div << std::endl;
		tune_log << std::endl;

		// eval_space
		acout() << "eval_space" << std::endl;
		acout() << "space_threshold: " << evaluate::space_threshold << std::endl;
		acout() << "space_weight_mult: " << evaluate::space_weight_mult << std::endl;
		acout() << "pace_weight_div: " << evaluate::space_weight_div << std::endl;
		acout() << std::endl;

		tune_log << "eval_space" << std::endl;
		tune_log << "space_threshold: " << evaluate::space_threshold << std::endl;
		tune_log << "space_weight_mult: " << evaluate::space_weight_mult << std::endl;
		tune_log << "space_weight_div: " << evaluate::space_weight_div << std::endl;
		tune_log << std::endl;

		// eval_strong_squares
		acout() << "eval_strong_squares" << std::endl;
		acout() << "safety_for_pawn_rbp: " << evaluate::safety_for_pawn_rbp << std::endl;
		acout() << "strong_p_in_front_of_pawn: " << evaluate::strong_p_in_front_of_pawn << std::endl;
		acout() << "strong_square_pb: " << evaluate::strong_square_pb << std::endl;
		acout() << "strong_square_pb_extra: " << evaluate::strong_square_pb_extra << std::endl;
		acout() << "pb_behind_pawn: " << evaluate::pb_behind_pawn << std::endl;
		acout() << "protected_piece: " << evaluate::protected_piece << std::endl;
		acout() << std::endl;

		tune_log << "eval_strong_squares" << std::endl;
		tune_log << "safety_for_pawn_rbp: " << evaluate::safety_for_pawn_rbp << std::endl;
		tune_log << "strong_p_in_front_of_pawn: " << evaluate::strong_p_in_front_of_pawn << std::endl;
		tune_log << "strong_square_pb: " << evaluate::strong_square_pb << std::endl;
		tune_log << "strong_square_pb_extra: " << evaluate::strong_square_pb_extra << std::endl;
		tune_log << "pb_behind_pawn: " << evaluate::pb_behind_pawn << std::endl;
		tune_log << "protected_piece: " << evaluate::protected_piece << std::endl;
		tune_log << std::endl;

		// eval_threats
		acout() << "eval_threats" << std::endl;
		acout() << "hanging_pawn_threat: " << evaluate::hanging_pawn_threat << std::endl;
		acout() << "hanging_pieces: " << evaluate::hanging_pieces << std::endl;
		acout() << "king_threat_single: " << evaluate::king_threat_single << std::endl;
		acout() << "king_threat_multiple: " << evaluate::king_threat_multiple << std::endl;
		acout() << "pawn_advance: " << evaluate::pawn_advance << std::endl;
		acout() << std::endl;

		tune_log << "eval_threats" << std::endl;
		tune_log << "hanging_pawn_threat: " << evaluate::hanging_pawn_threat << std::endl;
		tune_log << "hanging_pieces: " << evaluate::hanging_pieces << std::endl;
		tune_log << "king_threat_single: " << evaluate::king_threat_single << std::endl;
		tune_log << "king_threat_multiple: " << evaluate::king_threat_multiple << std::endl;
		tune_log << "pawn_advance: " << evaluate::pawn_advance << std::endl;
		tune_log << std::endl;

		// eval
		acout() << "eval" << std::endl;
		acout() << "blocked_pawns_mg: " << evaluate::blocked_pawns_mg << std::endl;
		acout() << "blocked_pawns_eg: " << evaluate::blocked_pawns_eg << std::endl;
		acout() << "mg_mgvalue_mult: " << evaluate::mg_mgvalue_mult << std::endl;
		acout() << "mg_egvalue_mult: " << evaluate::mg_egvalue_mult << std::endl;
		acout() << "eg_mgvalue_mult: " << evaluate::eg_mgvalue_mult << std::endl;
		acout() << "eg_egvalue_mult: " << evaluate::eg_egvalue_mult << std::endl;
		acout() << "eval_mult: " << evaluate::eval_mult << std::endl;
		acout() << "conversion_mult: " << evaluate::conversion_mult << std::endl;
		acout() << "conversion_div: " << evaluate::conversion_div << std::endl;
		acout() << "eval_div: " << evaluate::eval_div << std::endl;
		acout() << "eval_value_div: " << evaluate::eval_value_div << std::endl;
		acout() << "flank_double_attack: " << evaluate::flank_double_attack << std::endl;
		acout() << "pawn_contempt_mult: " << evaluate::pawn_contempt_mult << std::endl;
		acout() << "knight_contempt_mult: " << evaluate::knight_contempt_mult << std::endl;
		acout() << "bishop_contempt_mult: " << evaluate::bishop_contempt_mult << std::endl;
		acout() << "rook_contempt_mult: " << evaluate::rook_contempt_mult << std::endl;
		acout() << "queen_contempt_mult: " << evaluate::queen_contempt_mult << std::endl;
		acout() << "contempt_mult: " << evaluate::contempt_mult << std::endl;
		acout() << std::endl;

		tune_log << "eval" << std::endl;
		tune_log << "blocked_pawns_mg: " << evaluate::blocked_pawns_mg << std::endl;
		tune_log << "blocked_pawns_eg: " << evaluate::blocked_pawns_eg << std::endl;
		tune_log << "mg_mgvalue_mult: " << evaluate::mg_mgvalue_mult << std::endl;
		tune_log << "mg_egvalue_mult: " << evaluate::mg_egvalue_mult << std::endl;
		tune_log << "eg_mgvalue_mult: " << evaluate::eg_mgvalue_mult << std::endl;
		tune_log << "eg_egvalue_mult: " << evaluate::eg_egvalue_mult << std::endl;
		tune_log << "eval_mult: " << evaluate::eval_mult << std::endl;
		tune_log << "conversion_mult: " << evaluate::conversion_mult << std::endl;
		tune_log << "conversion_div: " << evaluate::conversion_div << std::endl;
		tune_log << "eval_div: " << evaluate::eval_div << std::endl;
		tune_log << "eval_value_div: " << evaluate::eval_value_div << std::endl;
		tune_log << "flank_double_attack: " << evaluate::flank_double_attack << std::endl;
		tune_log << "pawn_contempt_mult: " << evaluate::pawn_contempt_mult << std::endl;
		tune_log << "knight_contempt_mult: " << evaluate::knight_contempt_mult << std::endl;
		tune_log << "bishop_contempt_mult: " << evaluate::bishop_contempt_mult << std::endl;
		tune_log << "rook_contempt_mult: " << evaluate::rook_contempt_mult << std::endl;
		tune_log << "queen_contempt_mult: " << evaluate::queen_contempt_mult << std::endl;
		tune_log << "contempt_mult: " << evaluate::contempt_mult << std::endl;
		tune_log << std::endl;

		// piece_threat
		acout() << "piece_threat" << std::endl;
		acout() << "piece_threat: {"; for (auto& w : evaluate::piece_threat) acout() << w << ","; acout() << "}" << std::endl;
		acout() << "rook_threat: {"; for (auto& w : evaluate::rook_threat) acout() << w << ","; acout() << "}" << std::endl;
		acout() << "pawn_threat: {"; for (auto& w : evaluate::pawn_threat) acout() << w << ","; acout() << "}" << std::endl;
		acout() << std::endl;

		tune_log << "piece_threat" << std::endl;
		tune_log << "piece_threat: {"; for (auto& w : evaluate::piece_threat) tune_log << w << ","; tune_log << "}" << std::endl;
		tune_log << "rook_threat: {"; for (auto& w : evaluate::rook_threat) tune_log << w << ","; tune_log << "}" << std::endl;
		tune_log << "pawn_threat: {"; for (auto& w : evaluate::pawn_threat) tune_log << w << ","; tune_log << "}" << std::endl;
		tune_log << std::endl;

		// bishop pin
		acout() << "bishop pin" << std::endl;
		acout() << "bishop pin white: {"; for (auto w{0}; w < num_ranks; ++w) acout() << evaluate::bishop_pin[white][w] << ","; acout() << "}" << std::endl;
		acout() << "bishop pin black: {"; for (auto w{0}; w < num_ranks; ++w) acout() << evaluate::bishop_pin[black][w] << ","; acout() << "}" << std::endl;
		acout() << std::endl;

		tune_log << "bishop pin" << std::endl;
		tune_log << "bishop pin white: {"; for (auto w{0}; w < num_ranks; ++w) tune_log << evaluate::bishop_pin[white][w] << ","; tune_log << "}" << std::endl;
		tune_log << "bishop pin black: {"; for (auto w{0}; w < num_ranks; ++w) tune_log << evaluate::bishop_pin[black][w] << ","; tune_log << "}" << std::endl;
		tune_log << std::endl;

		// mobility tables
		acout() << "mobility tables" << std::endl;
		acout() << "mob_mult_const: {"; for (auto& w : evaluate::mob_mult_const) acout() << w << ","; acout() << "}" << std::endl;
		acout() << "mob_mult_rank_quad: {"; for (auto& w : evaluate::mob_mult_rank_quad) acout() << w << ","; acout() << "}" << std::endl;
		acout() << "mob_mult_file_quad: {"; for (auto& w : evaluate::mob_mult_file_quad) acout() << w << ","; acout() << "}" << std::endl;
		acout() << "mob_mult_center: {"; for (auto& w : evaluate::mob_mult_center) acout() << w << ","; acout() << "}" << std::endl;
		acout() << "mob_mult_rank: {"; for (auto& w : evaluate::mob_mult_rank) acout() << w << ","; acout() << "}" << std::endl;
		acout() << "mob_mult_edge: {"; for (auto& w : evaluate::mob_mult_edge) acout() << w << ","; acout() << "}" << std::endl;
		acout() << std::endl;

		tune_log << "mobility tables" << std::endl;
		tune_log << "mob_mult_const: {"; for (auto& w : evaluate::mob_mult_const) tune_log << w << ","; tune_log << "}" << std::endl;
		tune_log << "mob_mult_rank_quad: {"; for (auto& w : evaluate::mob_mult_rank_quad) tune_log << w << ","; tune_log << "}" << std::endl;
		tune_log << "mob_mult_file_quad: {"; for (auto& w : evaluate::mob_mult_file_quad) tune_log << w << ","; tune_log << "}" << std::endl;
		tune_log << "mob_mult_center: {"; for (auto& w : evaluate::mob_mult_center) tune_log << w << ","; tune_log << "}" << std::endl;
		tune_log << "mob_mult_rank: {"; for (auto& w : evaluate::mob_mult_rank) tune_log << w << ","; tune_log << "}" << std::endl;
		tune_log << "mob_mult_edge: {"; for (auto& w : evaluate::mob_mult_edge) tune_log << w << ","; tune_log << "}" << std::endl;
		tune_log << std::endl;

		// king danger table
		acout() << "king danger table" << std::endl;
		acout() << "king_danger: {"; for (auto& w : evaluate::king_danger) acout() << w << ","; acout() << "}" << std::endl;
		acout() << std::endl;

		tune_log << "king danger table" << std::endl;
		tune_log << "king_danger: {"; for (auto& w : evaluate::king_danger) tune_log << w << ","; tune_log << "}" << std::endl;
		tune_log << std::endl;

		// pawn eval
		// eval_pawns
		acout() << "eval_pawns" << std::endl;
		acout() << "center_bind: " << pawn::center_bind << std::endl;
		acout() << "multiple_passed_pawns: " << pawn::multiple_passed_pawns << std::endl;
		acout() << "second_row_fixed: " << pawn::second_row_fixed << std::endl;
		acout() << std::endl;

		tune_log << "eval_pawns" << std::endl;
		tune_log << "center_bind: " << pawn::center_bind << std::endl;
		tune_log << "multiple_passed_pawns: " << pawn::multiple_passed_pawns << std::endl;
		tune_log << "second_row_fixed: " << pawn::second_row_fixed << std::endl;
		tune_log << std::endl;

		// init
		acout() << "init" << std::endl;
		acout() << "max_safety_bonus: " << pawn::max_safety_bonus << std::endl;
		acout() << "pawn_unsupported: " << pawn::pawn_unsupported << std::endl;
		acout() << "chain_mult: " << pawn::chain_mult << std::endl;
		acout() << "chain_div: " << pawn::chain_div << std::endl;
		acout() << "file_factor_mult: " << pawn::file_factor_mult << std::endl;
		acout() << std::endl;

		tune_log << "init" << std::endl;
		tune_log << "max_safety_bonus: " << pawn::max_safety_bonus << std::endl;
		tune_log << "pawn_unsupported: " << pawn::pawn_unsupported << std::endl;
		tune_log << "chain_mult: " << pawn::chain_mult << std::endl;
		tune_log << "chain_div: " << pawn::chain_div << std::endl;
		tune_log << "king_1st_rank: " << pawn::king_1st_rank << std::endl;
		tune_log << "king_near_enemy_pawns: " << pawn::king_near_enemy_pawns << std::endl;
		tune_log << "file_factor_mult: " << pawn::file_factor_mult << std::endl;
		tune_log << std::endl;

		// eval_shelter_storm
		acout() << "eval_shelter_storm" << std::endl;
		acout() << "ss_base: " << pawn::ss_base << std::endl;
		acout() << "ss_safety_factor: " << pawn::ss_safety_factor << std::endl;
		acout() << "ss_danger_factor: " << pawn::ss_danger_factor << std::endl;
		acout() << std::endl;

		tune_log << "eval_shelter_storm" << std::endl;
		tune_log << "ss_base: " << pawn::ss_base << std::endl;
		tune_log << "ss_safety_factor: " << pawn::ss_safety_factor << std::endl;
		tune_log << "ss_danger_factor: " << pawn::ss_danger_factor << std::endl;
		tune_log << std::endl;

		// calculate_king_safety
		acout() << "calculate_king_safety" << std::endl;
		acout() << "safe_bonus_div: " << pawn::safe_bonus_div << std::endl;
		acout() << "safe_bonus_mult: " << pawn::safe_bonus_mult << std::endl;
		acout() << "safe_bonus_mult_r34: " << pawn::safe_bonus_mult_r34 << std::endl;
		acout() << "safe_bonus_mult_r5: " << pawn::safe_bonus_mult_r5 << std::endl;
		acout() << "king_1st_rank: " << pawn::king_1st_rank << std::endl;
		acout() << "king_near_enemy_pawns: " << pawn::king_near_enemy_pawns << std::endl;
		acout() << std::endl;

		tune_log << "calculate_king_safety" << std::endl;
		tune_log << "safe_bonus_div: " << pawn::safe_bonus_div << std::endl;
		tune_log << "safe_bonus_mult: " << pawn::safe_bonus_mult << std::endl;
		tune_log << "safe_bonus_mult_r34: " << pawn::safe_bonus_mult_r34 << std::endl;
		tune_log << "safe_bonus_mult_r5: " << pawn::safe_bonus_mult_r5 << std::endl;
		tune_log << "king_1st_rank: " << pawn::king_1st_rank << std::endl;
		tune_log << "king_near_enemy_pawns: " << pawn::king_near_enemy_pawns << std::endl;
		tune_log << std::endl;

		// ps factors
		acout() << "ps factors" << std::endl;
		acout() << "pp_ab_base_mg: " << evaluate::pp_ab_base_mg << std::endl;
		acout() << "mg_mg_mult: " << pawn::mg_mg_mult << std::endl;
		acout() << "mg_eg_mult: " << pawn::mg_eg_mult << std::endl;
		acout() << "eg_mg_mult: " << pawn::eg_mg_mult << std::endl;
		acout() << "eg_eg_mult: " << pawn::eg_eg_mult << std::endl;
		acout() << "ps_div: " << pawn::ps_div << std::endl;
		acout() << std::endl;

		tune_log << "ps factors" << std::endl;
		tune_log << "mg_mg_mult: " << pawn::mg_mg_mult << std::endl;
		tune_log << "mg_eg_mult: " << pawn::mg_eg_mult << std::endl;
		tune_log << "eg_mg_mult: " << pawn::eg_mg_mult << std::endl;
		tune_log << "eg_eg_mult: " << pawn::eg_eg_mult << std::endl;
		tune_log << "ps_div: " << pawn::ps_div << std::endl;
		tune_log << std::endl;

		// pawn shield/storm
		acout() << "pawn shield/storm" << std::endl;
		acout() << "shield_factor: {"; for (auto& w : pawn::shield_factor) acout() << w << ","; acout() << "}" << std::endl;
		acout() << "storm_factor: {"; for (auto& w : pawn::storm_factor) acout() << w << ","; acout() << "}" << std::endl;
		acout() << "pawn_storm_constants: {"; for (auto& w : pawn::pawn_storm_constants) acout() << w << ","; acout() << "}" << std::endl;
		acout() << std::endl;

		tune_log << "pawn shield/storm" << std::endl;
		tune_log << "shield_factor: {"; for (auto& w : pawn::shield_factor) tune_log << w << ","; tune_log << "}" << std::endl;
		tune_log << "storm_factor: {"; for (auto& w : pawn::storm_factor) tune_log << w << ","; tune_log << "}" << std::endl;
		tune_log << "pawn_storm_constants: {"; for (auto& w : pawn::pawn_storm_constants) tune_log << w << ","; tune_log << "}" << std::endl;
		tune_log << std::endl;

		// pawn phalanx
		acout() << "pawn phalanx" << std::endl;
		acout() << "phalanx_seed: {"; for (auto& w : pawn::phalanx_seed) acout() << w << ","; acout() << "}" << std::endl;
		acout() << "seed: {"; for (auto& w : pawn::seed) acout() << w << ","; acout() << "}" << std::endl;
		acout() << "remaining_score: {"; for (auto& w : pawn::remaining_score) acout() << w << ","; acout() << "}" << std::endl;
		acout() << "un_supported_pawn: {"; for (auto& w : pawn::un_supported_pawn) acout() << w << ","; acout() << "}" << std::endl;
		acout() << "pawn_attacker_score: {"; for (auto& w : pawn::pawn_attacker_score) acout() << w << ","; acout() << "}" << std::endl;
		acout() << std::endl;

		tune_log << "pawn phalanx" << std::endl;
		tune_log << "phalanx_seed: {"; for (auto& w : pawn::phalanx_seed) tune_log << w << ","; tune_log << "}" << std::endl;
		tune_log << "seed: {"; for (auto& w : pawn::seed) tune_log << w << ","; tune_log << "}" << std::endl;
		tune_log << "remaining_score: {"; for (auto& w : pawn::remaining_score) tune_log << w << ","; tune_log << "}" << std::endl;
		tune_log << "un_supported_pawn: {"; for (auto& w : pawn::un_supported_pawn) tune_log << w << ","; tune_log << "}" << std::endl;
		tune_log << "pawn_attacker_score: {"; for (auto& w : pawn::pawn_attacker_score) tune_log << w << ","; tune_log << "}" << std::endl;
		tune_log << std::endl;

		// passed pawn
		acout() << "passed pawn" << std::endl;
		acout() << "passed_pawn_values: {"; for (auto& w : pawn::passed_pawn_values) acout() << w << ","; acout() << "}" << std::endl;
		acout() << "passed_pawn_values_2: {"; for (auto& w : pawn::passed_pawn_values_2) acout() << w << ","; acout() << "}" << std::endl;
		acout() << std::endl;

		tune_log << "passed pawn" << std::endl;
		tune_log << "passed_pawn_values: {"; for (auto& w : pawn::passed_pawn_values) tune_log << w << ","; tune_log << "}" << std::endl;
		tune_log << "passed_pawn_values_2: {"; for (auto& w : pawn::passed_pawn_values_2) tune_log << w << ","; tune_log << "}" << std::endl;
		tune_log << std::endl;

		// doubled/isolated pawn
		acout() << "doubled/isolated pawn" << std::endl;
		acout() << "doubled_pawn: {"; for (auto& w : pawn::doubled_pawn) acout() << w << ","; acout() << "}" << std::endl;
		acout() << "isolated pawn white: {"; for (auto w{0}; w < num_files; ++w) acout() << pawn::isolated_pawn[white][w] << ","; acout() << "}" << std::endl;
		acout() << "isolated pawn black: {"; for (auto w{0}; w < num_files; ++w) acout() << pawn::isolated_pawn[black][w] << ","; acout() << "}" << std::endl;
		acout() << std::endl;

		tune_log << "doubled_pawn: {"; for (auto& w : pawn::doubled_pawn) tune_log << w << ","; tune_log << "}" << std::endl;
		tune_log << "isolated pawn white: {"; for (auto w{0}; w < num_files; ++w) tune_log << pawn::isolated_pawn[white][w] << ","; tune_log << "}" << std::endl;
		tune_log << "isolated pawn black: {"; for (auto w{0}; w < num_files; ++w) tune_log << pawn::isolated_pawn[black][w] << ","; tune_log << "}" << std::endl;
		tune_log << std::endl;

		// pawn tables
		acout() << "shelter_weakness" << std::endl;
		acout() << "shelter weakness: {"; for (auto w{0}; w < num_ranks; ++w) acout() << pawn::shelter_weakness[0][w] << ","; acout() << "}" << std::endl;
		acout() << "shelter weakness: {"; for (auto w{0}; w < num_ranks; ++w) acout() << pawn::shelter_weakness[1][w] << ","; acout() << "}" << std::endl;
		acout() << "shelter weakness: {"; for (auto w{0}; w < num_ranks; ++w) acout() << pawn::shelter_weakness[2][w] << ","; acout() << "}" << std::endl;
		acout() << "shelter weakness: {"; for (auto w{0}; w < num_ranks; ++w) acout() << pawn::shelter_weakness[3][w] << ","; acout() << "}" << std::endl;
		acout() << std::endl;

		tune_log << "shelter_weakness" << std::endl;
		tune_log << "shelter weakness: {"; for (auto w{0}; w < num_ranks; ++w) tune_log << pawn::shelter_weakness[0][w] << ","; tune_log << "}" << std::endl;
		tune_log << "shelter weakness: {"; for (auto w{0}; w < num_ranks; ++w) tune_log << pawn::shelter_weakness[1][w] << ","; tune_log << "}" << std::endl;
		tune_log << "shelter weakness: {"; for (auto w{0}; w < num_ranks; ++w) tune_log << pawn::shelter_weakness[2][w] << ","; tune_log << "}" << std::endl;
		tune_log << "shelter weakness: {"; for (auto w{0}; w < num_ranks; ++w) tune_log << pawn::shelter_weakness[3][w] << ","; tune_log << "}" << std::endl;
		tune_log << std::endl;

		acout() << "storm_danger" << std::endl;
		acout() << "storm danger: {"; for (auto w{0}; w < num_ranks; ++w) acout() << pawn::storm_danger[0][0][w] << ","; acout() << "}" << std::endl;
		acout() << "storm danger: {"; for (auto w{0}; w < num_ranks; ++w) acout() << pawn::storm_danger[0][1][w] << ","; acout() << "}" << std::endl;
		acout() << "storm danger: {"; for (auto w{0}; w < num_ranks; ++w) acout() << pawn::storm_danger[0][2][w] << ","; acout() << "}" << std::endl;
		acout() << "storm danger: {"; for (auto w{0}; w < num_ranks; ++w) acout() << pawn::storm_danger[0][3][w] << ","; acout() << "}" << std::endl;
		acout() << std::endl;

		tune_log << "storm_danger" << std::endl;
		tune_log << "storm danger: {"; for (auto w{0}; w < num_ranks; ++w) tune_log << pawn::storm_danger[0][0][w] << ","; tune_log << "}" << std::endl;
		tune_log << "storm danger: {"; for (auto w{0}; w < num_ranks; ++w) tune_log << pawn::storm_danger[0][1][w] << ","; tune_log << "}" << std::endl;
		tune_log << "storm danger: {"; for (auto w{0}; w < num_ranks; ++w) tune_log << pawn::storm_danger[0][2][w] << ","; tune_log << "}" << std::endl;
		tune_log << "storm danger: {"; for (auto w{0}; w < num_ranks; ++w) tune_log << pawn::storm_danger[0][3][w] << ","; tune_log << "}" << std::endl;
		tune_log << std::endl;

		// material
		// pawn factors
		acout() << "pawn factors" << std::endl;
		acout() << "p_base_score: " << material::p_base_score << std::endl;
		acout() << "p_q_factor: " << material::p_q_factor << std::endl;
		acout() << "p_r_factor: " << material::p_r_factor << std::endl;
		acout() << "p_b_factor: " << material::p_b_factor << std::endl;
		acout() << "p_n_factor: " << material::p_n_factor << std::endl;
		acout() << std::endl;

		tune_log << "pawn factors" << std::endl;
		tune_log << "p_base_score: " << material::p_base_score << std::endl;
		tune_log << "p_q_factor: " << material::p_q_factor << std::endl;
		tune_log << "p_r_factor: " << material::p_r_factor << std::endl;
		tune_log << "p_b_factor: " << material::p_b_factor << std::endl;
		tune_log << "p_n_factor: " << material::p_n_factor << std::endl;
		tune_log << std::endl;

		// knight factors
		acout() << "knight factors" << std::endl;
		acout() << "n_base_score: " << material::n_base_score << std::endl;
		acout() << "n_q_factor: " << material::n_q_factor << std::endl;
		acout() << "n_r_factor: " << material::n_r_factor << std::endl;
		acout() << "n_b_factor: " << material::n_b_factor << std::endl;
		acout() << "n_n_factor: " << material::n_n_factor << std::endl;
		acout() << "n_p_factor: " << material::n_p_factor << std::endl;
		acout() << std::endl;

		tune_log << "knight factors" << std::endl;
		tune_log << "n_base_score: " << material::n_base_score << std::endl;
		tune_log << "n_q_factor: " << material::n_q_factor << std::endl;
		tune_log << "n_r_factor: " << material::n_r_factor << std::endl;
		tune_log << "n_b_factor: " << material::n_b_factor << std::endl;
		tune_log << "n_n_factor: " << material::n_n_factor << std::endl;
		tune_log << "n_p_factor: " << material::n_p_factor << std::endl;
		tune_log << std::endl;

		// bishop factors
		acout() << "bishop factors" << std::endl;
		acout() << "b_base_score: " << material::b_base_score << std::endl;
		acout() << "b_q_factor: " << material::b_q_factor << std::endl;
		acout() << "b_r_factor: " << material::b_r_factor << std::endl;
		acout() << "b_b_factor: " << material::b_b_factor << std::endl;
		acout() << "b_n_factor: " << material::b_n_factor << std::endl;
		acout() << "b_p_factor: " << material::b_p_factor << std::endl;
		acout() << std::endl;

		tune_log << "bishop factors" << std::endl;
		tune_log << "b_base_score: " << material::b_base_score << std::endl;
		tune_log << "b_q_factor: " << material::b_q_factor << std::endl;
		tune_log << "b_r_factor: " << material::b_r_factor << std::endl;
		tune_log << "b_b_factor: " << material::b_b_factor << std::endl;
		tune_log << "b_n_factor: " << material::b_n_factor << std::endl;
		tune_log << "b_p_factor: " << material::b_p_factor << std::endl;
		tune_log << std::endl;

		// rook factors
		acout() << "rook factors" << std::endl;
		acout() << "r_base_score: " << material::r_base_score << std::endl;
		acout() << "r_q_factor: " << material::r_q_factor << std::endl;
		acout() << "r_r_factor: " << material::r_r_factor << std::endl;
		acout() << "r_b_factor: " << material::r_b_factor << std::endl;
		acout() << "r_n_factor: " << material::r_n_factor << std::endl;
		acout() << "r_p_factor: " << material::r_p_factor << std::endl;
		acout() << std::endl;

		tune_log << "rook factors" << std::endl;
		tune_log << "r_base_score: " << material::r_base_score << std::endl;
		tune_log << "r_q_factor: " << material::r_q_factor << std::endl;
		tune_log << "r_r_factor: " << material::r_r_factor << std::endl;
		tune_log << "r_b_factor: " << material::r_b_factor << std::endl;
		tune_log << "r_n_factor: " << material::r_n_factor << std::endl;
		tune_log << "r_p_factor: " << material::r_p_factor << std::endl;
		tune_log << std::endl;

		// queen factors
		acout() << "queen factors" << std::endl;
		acout() << "q_base_score: " << material::q_base_score << std::endl;
		acout() << "q_q_factor: " << material::q_q_factor << std::endl;
		acout() << "q_r_factor: " << material::q_r_factor << std::endl;
		acout() << "q_b_factor: " << material::q_b_factor << std::endl;
		acout() << "q_n_factor: " << material::q_n_factor << std::endl;
		acout() << "q_p_factor: " << material::q_p_factor << std::endl;
		acout() << std::endl;

		tune_log << "queen factors" << std::endl;
		tune_log << "q_base_score: " << material::q_base_score << std::endl;
		tune_log << "q_q_factor: " << material::q_q_factor << std::endl;
		tune_log << "q_r_factor: " << material::q_r_factor << std::endl;
		tune_log << "q_b_factor: " << material::q_b_factor << std::endl;
		tune_log << "q_n_factor: " << material::q_n_factor << std::endl;
		tune_log << "q_p_factor: " << material::q_p_factor << std::endl;
		tune_log << std::endl;

		// bishop pair factors
		acout() << "bishop pair factors" << std::endl;
		acout() << "bp_base_score: " << material::bp_base_score << std::endl;
		acout() << "bp_q_factor: " << material::bp_q_factor << std::endl;
		acout() << "bp_r_factor: " << material::bp_r_factor << std::endl;
		acout() << "bp_b_factor: " << material::bp_b_factor << std::endl;
		acout() << "bp_n_factor: " << material::bp_n_factor << std::endl;
		acout() << std::endl;

		tune_log << "bishop pair factors" << std::endl;
		tune_log << "bp_base_score: " << material::bp_base_score << std::endl;
		tune_log << "bp_q_factor: " << material::bp_q_factor << std::endl;
		tune_log << "bp_r_factor: " << material::bp_r_factor << std::endl;
		tune_log << "bp_b_factor: " << material::bp_b_factor << std::endl;
		tune_log << "bp_n_factor: " << material::bp_n_factor << std::endl;
		tune_log << std::endl;

		// material imbalance
		acout() << "material imbalance" << std::endl;
		acout() << "more_knights_bonus: " << material::more_knights_bonus << std::endl;
		acout() << "more_bishops_bonus: " << material::more_bishops_bonus << std::endl;
		acout() << "more_knights_bonus: " << material::more_knights_bonus << std::endl;
		acout() << std::endl;

		tune_log << "material imbalance" << std::endl;
		tune_log << "more_knights_bonus: " << material::more_knights_bonus << std::endl;
		tune_log << "more_bishops_bonus: " << material::more_bishops_bonus << std::endl;
		tune_log << "more_knights_bonus: " << material::more_knights_bonus << std::endl;
		tune_log << std::endl;

		// phase factors
		acout() << "phase factors" << std::endl;
		acout() << "max_phase: " << material::max_phase << std::endl;
		acout() << "r_phase_factor: " << material::r_phase_factor << std::endl;
		acout() << "q_phase_factor: " << material::q_phase_factor << std::endl;
		acout() << std::endl;

		tune_log << "phase factors" << std::endl;
		tune_log << "max_phase: " << material::max_phase << std::endl;
		tune_log << "r_phase_factor: " << material::r_phase_factor << std::endl;
		tune_log << "q_phase_factor: " << material::q_phase_factor << std::endl;
		tune_log << std::endl;

		tune_log.close();
		acout() << "saved " << file_name << std::endl;
		acout() << std::endl;
	}

	double sigmoid(const double k, const int sc)
	{
		// calculating sigmoid function
		return 1.0 / (1.0 + std::pow(10.0, -k * sc / 400.0));
	}

	void eval_error_range(const std::vector<texel_position>& texel_pos, std::atomic<double>& error, const double k, const int range_min, const int range_max)
	{
		// calculating the average evaluation error of a range
		double error_range{};

		for (auto i = range_min; i < range_max; ++i)
		{
			const auto sc
			{
				evaluate::eval(texel_pos[i].pos, no_score, no_score) * (texel_pos[i].pos.on_move() == white ? 1 : -1)
			};
			error_range += std::pow(texel_pos[i].result - sigmoid(k, sc), 2);
		}
		error = error + error_range;
	}

	double eval_error(const std::vector<texel_position>& texel_pos, double k, const int thread_cnt)
	{
		// calculating the average evaluation error
		std::atomic<double> error{};
		const auto share
		{
			texel_pos.size() / static_cast<double>(thread_cnt)
		};
		double range_min{};

		// using multiple threads here because this is the speed-limiting step
		std::vector<std::thread> threads(thread_cnt);
		for (auto& t : threads)
		{
			t = std::thread
			{
				eval_error_range, std::ref(texel_pos), std::ref(error), k, static_cast<int>(std::floor(range_min)),
				static_cast<int>(std::floor(range_min + share) - 1)
			};
			range_min += share;
		}
		for (auto& t : threads)
			t.join();

		return error / static_cast<double>(texel_pos.size());
	}

	double optimal_k(const std::vector<texel_position>& texel_pos, const int thread_cnt)
	{
		// computing the optimal scaling constant K
		double k_best{};
		auto error_min = std::numeric_limits<double>::max();
		std::cout.precision(k_precision + 1);

		for (int i{}; i <= k_precision; ++i)
		{
			//acout() << "iteration " << i + 1 << ": ";
			const auto unit = std::pow(10.0, -i);
			const auto range = unit * 10.0;
			const auto k_max = k_best + range;

			for (auto k = std::max(k_best - range, 0.0); k <= k_max; k += unit)
			{
				const auto error
				{
					eval_error(texel_pos, k, thread_cnt)
				};
				if (error < error_min)
				{
					error_min = error;
					k_best = k;
				}
			}
		}
		acout() << "K = " << std::fixed << std::setprecision(2) << k_best << std::endl;
		return k_best;
	}

	bool smaller_error(const std::vector<int>& weights, const std::vector<texel_position>& texel_pos, double& error_min, const double k, const int thread_cnt)
	{
		// calculating the new evaluation error & determining if it is smaller
		save_weights(weights);
		const auto error = eval_error(texel_pos, k, thread_cnt);
		if (error <= error_min)
		{
			// checking the new error for sanity
			if (error_min - error > error_min / thread_cnt / 2)
			{
				calc_errors += 1;
				return false;
			}
			error_min = error;
			return true;
		}
		return false;
	}
}

void tuner::tune(std::string& epd_file, int thread_cnt)
{
	// tuning the evaluation parameters with positions from the epd_file
	std::vector<texel_position> texel_pos{};
	thread_cnt = std::max(1, thread_cnt);

	// loading positions from the epd_file
	load_pos(epd_file, texel_pos);

	acout() << std::endl;
	acout() << "reading fen strings from: " << epd_file << std::endl;
	acout() << std::endl;

	acout() << "positions loaded: " << texel_pos.size() << std::endl;
	std::cout.precision(2);
	acout() << "memory allocated: " << std::fixed << std::setprecision(1) << (size(texel_pos) * texel_pos.size() >> 20) / 806.2  << " MB" << std::endl;
	acout() << "threads utilized: " << thread_cnt << std::endl;
	acout() << std::endl;

	// loading current evaluation weights
	std::vector<int> weights{};
	weights.reserve(weight_cnt);
	load_weights(weights);
	acout() << "weights to tune: " << weights.size() << std::endl;

	if (weights.size() != weight_cnt)
		acout() << "weight count incorrect" << std::endl;
	acout() << std::endl;

	// computing optimal scaling constant K
	acout() << "computing optimal scaling constant K ..." << std::endl;
	const auto k
	{
		optimal_k(texel_pos, thread_cnt)
	};
	acout() << std::endl;

	// computing the evaluation error to start with
	acout() << "computing evaluation error constant E ..." << std::endl;
	const auto start_time = now();
	const auto error_start = eval_error(texel_pos, k, thread_cnt);
	const auto finish_time = now();
	std::cout.precision(4);
	acout() << "E = " << std::fixed << std::setprecision(2) << error_start << std::endl << std::endl;

	// starting with the tuning-iterations
	acout() << "tuning ..." << std::endl;

	// estimating iteration time
	acout() << "estimated tuning time: ";
	acout() << (static_cast<uint64_t>(finish_time) - start_time) * weights.size() / 1000 << " sec" << std::endl;

	auto error_min = error_start;
	calc_errors = 0;

	for (auto i = 1; true; ++i)
	{
		acout() << std::endl;
		acout() << "iteration " << i << " ...";

		const auto error_curr = error_min;

		for (auto& w : weights)
		{
			const auto save_weight = w;
			auto improve = false;

			w += 1;
			if (smaller_error(weights, texel_pos, error_min, k, thread_cnt))
				improve = true;
			else
			{
				w -= 2;
				improve = smaller_error(weights, texel_pos, error_min, k, thread_cnt);
			}
			if (!improve)
				w = save_weight;
		}
		acout() << std::endl;
		acout() << "evaluation error: " << error_min << std::endl;
		save_weights(weights);

		acout() << std::endl;
		display_weights();

		if (static_cast<int>(error_min) == static_cast<int>(error_curr))
			break;
	}

	// finishing tuning
	acout() << "tuning time: " << (now() - start_time) / 1000 << " secs";
	acout() << std::endl;
	acout() << "calculation errors: " << calc_errors;
	acout() << std::endl;
	acout() << "evaluation error: " << error_start << " -> " << error_min;
	acout() << std::endl;
	acout() << "tuning completed" << std::endl;
}
#endif
