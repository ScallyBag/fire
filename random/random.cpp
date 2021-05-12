#include <random>
#include "../position.h"
#include "../movepick.h"
#include "../pragma.h"
#include "../util/misc.h"

// use uniform_real_distribution to generate all possible value and avoid statistical bias
void random(position& pos) {

	int num_moves = 0;
	
    // find & count legal moves
	for (const auto& m : legal_move_list(pos))
		num_moves++;		

	// seed the generator	
	std::random_device rd;
	
	// use standard mersenne_twister_engine
    std::mt19937 gen(rd());
	
	// calculate a uniform distribution between 1 and num_moves + 1
	std::uniform_real_distribution<> distribution (1, num_moves + 1);
	
	// generate a random number using the distribution from above
    const int r = static_cast<int>(distribution(gen));
	
    // find the move
	num_moves = 0;
	for (const auto& m : legal_move_list(pos))
	{
		num_moves++;		
		if (num_moves == r)
		{
			// play and output the move
			pos.play_move(m, pos.give_check(m));
			acout() << "bestmove " << util::move_to_string(m, pos) << std::endl;
		}
	}	
}

