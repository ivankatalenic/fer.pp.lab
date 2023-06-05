#include "mpi.h"

#include <memory>
#include <vector>
#include <iostream>
#include <cmath>
#include <cstdint>

using namespace std;

constexpr int EMPTY{0};
constexpr int COMPUTER{1};
constexpr int HUMAN{2};

struct board {
	int width;
	int height;
	shared_ptr<int[]> mem;
	board():
		width{0}, height{0}, mem{nullptr} {};
	board(int w, int h):
		width{w}, height{h}, mem{new int[w * h]} {
		for (int i{0}; i < w * h; ++i) {
			mem[i] = EMPTY;
		}
	};
	int& operator()(int col, int row) {
		return mem[row + col * height];
	}
	board copy() {
		board other{};
		other.width = width;
		other.height = height;
		other.mem = shared_ptr<int[]>(new int[width * height]);
		for (int i{0}; i < width * height; ++i) {
			other.mem[i] = mem[i];
		}
	}
};

struct state {
	board b;
	int remaining_depth;
	// Column: 0 .. WIDTH-1
	int last_move_col;
	// Computer or human move
	int last_move_player;

	state() = default;
	state(board b, int rem_d, int col, int player):
		b{b},
		remaining_depth{rem_d},
		last_move_col{col},
		last_move_player{player} {};
	state(int w, int h, int rem_d, int col, int player):
		b(w, h),
		remaining_depth{rem_d},
		last_move_col{col},
		last_move_player{player} {};
};

struct node {
	state s;
	vector<shared_ptr<node>> children;

	node(state s): s{s} {};
};

int main(int argc, char* argv[]) {
	MPI_Init(&argc, &argv);

	int rank, size;
	MPI_Comm_rank(MPI_COMM_WORLD, &rank);
	MPI_Comm_size(MPI_COMM_WORLD, &size);

	const int max_depth{atoi(argv[1])};

	if (rank == 0) {
		// Read input
		state initial{read_state()};
		initial.remaining_depth = max_depth;
		// Compute neccessary sched tree depth
		const int sched_tree_depth{
			1 + ceil(log(size) / log(initial.b.width))
		};
		// Build the scheduling tree
		shared_ptr<node> root_node{make_shared<node>(initial)};
		build_sched_tree(root_node, sched_tree_depth);
		// Filter the sched tree
		// Send tasks
	} else {
		// Accept tasks
		// Calculate utility
		// Send utility to the root
	}

	MPI_Finalize();
	return 0;
}

state read_state() {
	int width, height;

	cin >> height >> width;
	board b(width, height);
	for (int j{0}; j < height; ++j) {
		for (int i{0}; i < width; ++i) {
			int field;
			cin >> field;
			b(i, j) = field;
		}
	}

	return state(b, 0, 0, HUMAN);
}

void build_sched_tree(shared_ptr<node> root, int max_depth) {
	if (max_depth == 1) {
		return;
	}
	for (int col{0}; col < root->s.b.width; ++col) {
		// Compute the next valid move
		int h{0};
		while (
			h < root->s.b.height
			&& root->s.b(col, h) != EMPTY
		) h++;
		if (h == root->s.b.height) {
			// The column is full, skip the column.
			continue;
		}
		// Compute the move player
		int next_player{COMPUTER};
		if (root->s.last_move_player == COMPUTER) {
			next_player = HUMAN;
		}
		board new_board{root->s.b.copy()};
		new_board(col, h) = next_player;
		state new_state{state(
			new_board,
			root->s.remaining_depth - 1,
			col,
			next_player
		)};
		shared_ptr<node> new_node{make_shared<node>(new_state)};
		root->children.push_back(new_node);
		build_sched_tree(new_node, max_depth - 1);
	}
}
