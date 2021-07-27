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

#include <algorithm>
#include <cassert>
#include <unordered_map>

#include "node.h"
#include "mcts.h"

#include "../evaluate.h"
#include "../fire.h"
#include "../position.h"
#include "../pragma.h"
#include "../search.h"
#include "../thread.h"
#include "../uci.h"

mcts_hash_table mcts;

uint32_t monte_carlo::search()
{
	int depth = 0;
	create_root();
	search::running = true;
	thread_pool.root_position->copy_position(thread_pool.root_position, nullptr, nullptr);
	const auto me = thread_pool.root_position->on_move();
	time_control.init(search::param, me, thread_pool.root_position->game_ply());
	search::previous_info_time = 0;

	while (computational_budget())
	{
		mc_node node = tree_policy();
		const reward n_reward = playout_policy(node);
		backup(n_reward);

		if (uci_minimax == false && should_output_result())
		{
			depth++;
			print_pv(depth);
		}
	}

	if (uci_minimax == false)
	{
		depth++;
		print_pv(depth);
	}

	if (uci_minimax == false)
		print_children = true;

	return best_child(root_, stat_visits)->move;
}

bool monte_carlo::computational_budget() const
{
	assert(is_root(current_node()));

	if (pos_.my_thread() == thread_pool.main())
		static_cast<mainthread*>(pos_.my_thread())->check_time();

	return (descent_cnt_ < max_descents_)
		&& !search::signals.stop_analyzing.load(std::memory_order_relaxed);
}

void mainthread::check_time()
{
	const int time_denominator = 60;

	if (--calls_cnt > 0)
		return;

	calls_cnt = search::param.nodes ? std::min(1024, static_cast<int>(search::param.nodes / 1024)) : 1024;

	static time_point last_info_time = now();

	const time_point elapsed = time_control.elapsed();

	if (const time_point tick = search::param.start_time + elapsed; tick - last_info_time >= 1000)
	{
		last_info_time = tick;
	}

	if (search::param.ponder)
		return;

	if (const int time_numerator = 10; (search::param.use_time_calculating() && (elapsed > time_control.maximum() * time_numerator / time_denominator || search::signals.stop_if_ponder_hit))
		|| (search::param.move_time && elapsed >= search::param.move_time)
		|| (search::param.nodes && thread_pool.visited_nodes() >= search::param.nodes))
		search::signals.stop_analyzing = true;
}

int monte_carlo::minimax_value(position& pos, const int depth) const
{
	const int alpha = -max_score;
	const int beta = max_score;

	const int value = depth < 1 ? pos.is_in_check()
		? search::q_search<search::nodetype::PV, true>(pos, alpha, beta, depth_0)
		: search::q_search<search::nodetype::PV, false>(pos, alpha, beta, depth_0)
		: search::alpha_beta<search::nodetype::PV>(pos, beta - score_1, beta, depth_0, false);

	return value;
}

int monte_carlo::evaluate(const int depth) const
{
	int score = 0;

	if (uci_minimax == true)
		score = minimax_value(pos_, depth);
	else
		score = evaluate::eval(pos_, no_score, no_score);

	return score;
}










