#include "mpi.h"

#include <memory>
#include <vector>
#include <iostream>
#include <cmath>
#include <cstdint>
#include <initializer_list>
#include <array>
#include <stdexcept>
#include <cstddef>
#include <cstring>
#include <string>
#include <fstream>

constexpr uint8_t EMPTY{0};
constexpr uint8_t COMPUTER{1};
constexpr uint8_t HUMAN{2};

constexpr uint8_t TASK_TAG{0};
constexpr uint8_t END_TAG{1};
constexpr uint8_t UTILITY_TAG{2};

struct pair {
	int8_t x, y;
	
	pair(int8_t a, int8_t b): x{a}, y{b} {};
	pair(): x{0}, y{0} {};
	pair(std::initializer_list<int8_t> l) {
		auto iter{l.begin()};
		x = *iter++;
		y = *iter;
	};
	pair operator*(int8_t m) const { return pair(x * m, y * m); };
	pair operator+(int8_t a) const { return pair(x + a, y + a); };
	pair operator-(int8_t a) const { return pair(x - a, y - a); };
	pair operator+(const pair& other) const { return pair(x + other.x, y + other.y); };
	pair operator-(const pair& other) const { return pair(x - other.x, y - other.y); };
};

struct board {
	uint8_t width;
	uint8_t height;
	std::shared_ptr<uint8_t[]> mem;

	board():
		width{0}, height{0}, mem{nullptr} {};
	board(uint8_t w, uint8_t h):
		width{w}, height{h}, mem{new uint8_t[w * h]} {
		for (std::size_t i{0}; i < w * h; ++i) {
			mem[i] = EMPTY;
		}
	};
	uint8_t& operator()(uint8_t col, uint8_t row) {
		return mem[row + col * height];
	};
	uint8_t& operator()(pair loc) {
		return mem[loc.y + loc.x * height];
	};
	board copy() {
		board other;
		other.width = width;
		other.height = height;
		other.mem = std::shared_ptr<uint8_t[]>(new uint8_t[width * height]);
		for (std::size_t i{0}; i < width * height; ++i) {
			other.mem[i] = mem[i];
		}
		return other;
	};
	bool check_connect4(uint8_t col, uint8_t row) {
		const std::array<pair, 4> directions{pair{0, 1}, pair{1, 1}, pair{1, 0}, pair{1, -1}};
		const uint8_t player{operator()(col, row)};
		const pair pos{static_cast<int8_t>(col), static_cast<int8_t>(row)};
		for (pair dir : directions) {
			for (int8_t offset{0}; offset < 4; ++offset) {
				pair start{pos - dir * offset};
				if (start.x < 0 || start.x >= width || start.y < 0 || start.y >= height) {
					// Outside of the board
					continue;
				}
				bool check{true};
				for (int8_t i{0}; i < 4; ++i) {
					const pair elem{start + dir * i};
					if (elem.x < 0 || elem.x >= width || elem.y < 0 || elem.y >= height) {
						check = false;
						break;
					}
					check = check && (operator()(elem) == player);
				}
				if (check) {
					return true;
				}
			}
		}
		return false;
	};
	// Accepts possibly empty array and expands it
	void serialize(std::vector<std::byte>& arr) {
		const std::size_t size{(2 + width * height) * sizeof(uint8_t)};
		const std::size_t old_size{arr.size()};
		arr.resize(old_size + size, std::byte{0});
		uint8_t* ints{reinterpret_cast<uint8_t*>(arr.data() + old_size)};
		*ints++ = width;
		*ints++ = height;
		std::memcpy(ints, mem.get(), width * height);
	};
	std::size_t deserialize(const std::vector<std::byte>& arr) {
		const uint8_t* ints{reinterpret_cast<const uint8_t*>(arr.data())};
		width = *ints++;
		height = *ints++;
		mem = std::shared_ptr<uint8_t[]>(new uint8_t[width * height]);
		std::memcpy(mem.get(), ints, width * height);
		return (2 + width * height) * sizeof(uint8_t);
	};
};

struct state {
	board b;
	uint8_t remaining_depth;
	// Column: 0 .. WIDTH-1
	uint8_t last_move_col;
	// Computer or human move
	uint8_t last_move_player;
	double utility;

	state() = default;
	state(board b, uint8_t rem_d, uint8_t col, uint8_t player):
		b{b},
		remaining_depth{rem_d},
		last_move_col{col},
		last_move_player{player},
		utility{0.0} {};
	state(uint8_t w, uint8_t h, uint8_t rem_d, uint8_t col, uint8_t player):
		b(w, h),
		remaining_depth{rem_d},
		last_move_col{col},
		last_move_player{player},
		utility{0.0} {};
	// Accepts possibly empty array and expands it
	void serialize(std::vector<std::byte>& arr) {
		b.serialize(arr);
		const std::size_t size{3*sizeof(uint8_t) + sizeof(double)};
		const std::size_t old_size{arr.size()};
		arr.resize(old_size + size, std::byte{0});
		uint8_t* ints{reinterpret_cast<uint8_t*>(arr.data() + old_size)};
		*ints++ = remaining_depth;
		*ints++ = last_move_col;
		*ints++ = last_move_player;
		double* doubles{reinterpret_cast<double*>(ints)};
		*doubles = utility;
	};
	std::size_t deserialize(const std::vector<std::byte>& arr) {
		const std::size_t used{b.deserialize(arr)};
		const uint8_t* ints{reinterpret_cast<const uint8_t*>(arr.data() + used)};
		remaining_depth = *ints++;
		last_move_col = *ints++;
		last_move_player = *ints++;
		const double* doubles{reinterpret_cast<const double*>(ints)};
		utility = *doubles;
		return used + 3 * sizeof(uint8_t) + sizeof(double);
	};
};

struct node {
	state s;
	std::vector<std::shared_ptr<node>> children;

	node(state s): s{s} {};
	node() = default;
};

state read_state(const std::string filename);
// Expects a starting state that is not a win.
void build_sched_tree(std::shared_ptr<node> root, uint8_t sched_tree_depth, uint8_t max_depth);
void generate_tasks_rec(std::vector<std::shared_ptr<node>>& tasks, std::shared_ptr<node> root);
std::vector<std::shared_ptr<node>> generate_tasks(std::shared_ptr<node> root_node);
void send_task(const int id, std::shared_ptr<node> task);
void send_utility(double utility);
void send_end_signal(int id);
std::shared_ptr<node> receive_task();
double receive_utility(const int id);
double compute_utility(std::shared_ptr<node> root);
void distribute_tasks(std::vector<std::shared_ptr<node>>& tasks, int size);
void complete_computation(std::shared_ptr<node> root);
pair select_best_move(std::shared_ptr<node> root);
std::size_t count_nodes(std::shared_ptr<node> root);

int main(int argc, char* argv[]) {
	MPI_Init(&argc, &argv);

	int rank, size;
	MPI_Comm_rank(MPI_COMM_WORLD, &rank);
	MPI_Comm_size(MPI_COMM_WORLD, &size);

	const std::string input{argv[1]};
	const uint8_t max_depth{static_cast<uint8_t>(atoi(argv[2]))};
	uint8_t sched_depth;
	if (argc >= 4) {
		sched_depth = static_cast<uint8_t>(atoi(argv[3]));
	}

	if (rank == 0) {
		// Read input
		state initial{read_state(input)};
		initial.remaining_depth = max_depth - 1;
		initial.last_move_player = HUMAN;
		// Compute neccessary sched tree depth
		uint8_t sched_tree_depth{static_cast<uint8_t>(
			1 + ceil(log(size) / log(initial.b.width))
		)};
		if (argc >= 4) {
			sched_tree_depth = sched_depth;
		}
		if (sched_tree_depth >= max_depth) {
			throw std::runtime_error("sched_tree_depth can't be equal or greater than max depth");
		}

		std::cout << "scheduling tree depth: " << int(sched_tree_depth) << std::endl;

		// Build the scheduling tree
		std::shared_ptr<node> root_node{std::make_shared<node>(initial)};
		build_sched_tree(root_node, sched_tree_depth, max_depth);

			std::cout << "scheduling tree node count: " << count_nodes(root_node) << std::endl;

		// Generate tasks
		std::vector<std::shared_ptr<node>> tasks{generate_tasks(root_node)};

			std::cout << "tasks len: " << tasks.size() << std::endl;

		// Send tasks
		distribute_tasks(tasks, size);
		// Send the end signal
		for (int id{1}; id < size; ++id) {
			send_end_signal(id);
		}

			std::cout << "tree node count: " << count_nodes(root_node) << std::endl;

		// Finish the computation
		complete_computation(root_node);
		// Select the move
		pair move{select_best_move(root_node)};

		std::cout << "best move: col = " << move.x + 1 << std::endl;
		std::cout << "root utility: " << root_node->s.utility << std::endl;
	} else {
		while (true) {
			// Accept the task
			std::shared_ptr<node> task{receive_task()};
			if (task == nullptr) {
				MPI_Finalize();
				return 0;
			}
			// Calculate the utility
			const double utility{compute_utility(task)};
			// Send utility to the root
			std::cout << "worker " << rank << " sending utility " << utility << std::endl;
			send_utility(utility);
		}
	}

	MPI_Finalize();
	return 0;
}

std::size_t count_nodes(std::shared_ptr<node> root) {
	std::size_t subtree{1};
	for (std::shared_ptr<node> subnode : root->children) {
		subtree += count_nodes(subnode);
	}
	return subtree;
}

pair select_best_move(std::shared_ptr<node> root) {
	double best_utility{-1.0};
	pair best_move;
	for (std::shared_ptr<node> subnode : root->children) {
		if (subnode->s.utility >= best_utility) {
			best_utility = subnode->s.utility;
			uint8_t h{0};
			while (h < subnode->s.b.height && subnode->s.b(subnode->s.last_move_col, h) != EMPTY) {
				h++;
			}
			h--;
			best_move = pair(subnode->s.last_move_col, h);
		}
	}
	return best_move;
}

void complete_computation(std::shared_ptr<node> root) {
	if (root->s.utility != 0.0) {
		return;
	}
	double utility{0.0};
	double size{static_cast<double>(root->children.size())};
	for (std::shared_ptr<node> subnode : root->children) {
		complete_computation(subnode);
		utility += subnode->s.utility / size;
	}
	root->s.utility = utility;
}

void distribute_tasks(std::vector<std::shared_ptr<node>>& tasks, int size) {
	// Currently distributed tasks
	std::vector<std::shared_ptr<node>> id_work_task;
	for (int i{0}; i < size; ++i) {
		id_work_task.push_back(nullptr);
	}

	int task_cnt{0};
	while (!tasks.empty()) {
		// Send worker tasks
		for (int id{1}; id < size; ++id) {
			if (tasks.empty()) {
				id_work_task[id] = nullptr;
				break;
			}
			std::shared_ptr<node> task{tasks.back()};
			tasks.pop_back();

			id_work_task[id] = task;
			
			// Send the task
			send_task(id, task);

			std::cout << "sent task " << task_cnt++ << " to worker " << id << std::endl;
		}
		// Do root tasks
		if (!tasks.empty()) {
			std::shared_ptr<node> task{tasks.back()};
			tasks.pop_back();

			std::cout << "doing task " << task_cnt++ << " on root" << std::endl;

			// Do the task yourself
			const double utility{compute_utility(task)};

			// Erase subnodes so we don't repeat the computation later.
			task->children.clear();

			std::cout << "received utility " << utility << " from worker " << 0 << std::endl;

			// Update the node with the result
			task->s.utility = utility;
		}
		// Collect results
		for (int id{1}; id < size; ++id) {
			std::shared_ptr<node> task{id_work_task[id]};
			if (task == nullptr) {
				// No task for this worker
				continue;
			}
			// Wait for the result
			const double utility{receive_utility(id)};

			std::cout << "received utility " << utility << " from worker " << id << std::endl;

			// Update the node with the result
			task->s.utility = utility;
			
			// Reset the task
			id_work_task[id] = nullptr;
		}
	}
}

double compute_utility(std::shared_ptr<node> root) {
	if (root->s.remaining_depth <= 0) {
		return 0.0;
	}
	// Generate subnodes
	for (uint8_t col{0}; col < root->s.b.width; ++col) {
		// Compute the next valid move
		uint8_t h{0};
		while (
			h < root->s.b.height
			&& root->s.b(col, h) != EMPTY
		) h++;
		if (h == root->s.b.height) {
			// The column is full, skip the column.
			continue;
		}

		// Compute the player
		uint8_t next_player{COMPUTER};
		if (root->s.last_move_player == COMPUTER) {
			next_player = HUMAN;
		}

		// Compute the board
		board new_board{root->s.b.copy()};
		new_board(col, h) = next_player;

		// Compute the state
		state new_state(
			new_board,
			root->s.remaining_depth - 1,
			col,
			next_player
		);

		// Check if there's connect 4
		if (new_board.check_connect4(col, h)) {
			if (next_player == COMPUTER) {
				new_state.utility = 1.0;
				// The computer always selects a move (subnode) that wins the game.
				root->s.utility = 1.0;
			} else {
				new_state.utility = -1.0;
				// Human can make a mistake
			}
		}

		std::shared_ptr<node> new_node{std::make_shared<node>(new_state)};
		if (new_state.utility == 1.0) {
			// No need to consider other moves (subnodes).
			// Root now has the computed utility.
			root->children.clear();
			root->children.push_back(new_node);
			return 1.0;
		}

		root->children.push_back(new_node);
		if (new_state.utility == -1.0) {
			// Don't consider subnodes if a human won.
			continue;
		}

		new_node->s.utility = compute_utility(new_node);
	}

	double utility{0.0};
	double size{static_cast<double>(root->children.size())};
	for (std::shared_ptr<node> subnode : root->children) {
		utility += subnode->s.utility / size;
	}
	return utility;
}

double receive_utility(const int id) {
	double utility;
	MPI_Status status;
	if (MPI_Recv(&utility, 1, MPI_DOUBLE, id, MPI_ANY_TAG, MPI_COMM_WORLD, &status) != MPI_SUCCESS) {
		throw std::runtime_error("failed to receive a utility");
	}
	if (status.MPI_TAG != UTILITY_TAG) {
		throw std::runtime_error("received a different tag when expecting a utility tag");
	}
	return utility;
}

std::shared_ptr<node> receive_task() {
	MPI_Status status;
	if (MPI_Probe(0, MPI_ANY_TAG, MPI_COMM_WORLD, &status) != MPI_SUCCESS) {
		throw std::runtime_error("failed to probe");
	}
	if (status.MPI_TAG == END_TAG) {
		return nullptr;
	}

	int count;
	if (MPI_Get_count(&status, MPI_BYTE, &count) != MPI_SUCCESS) {
		throw std::runtime_error("failed to get count");
	}

	std::vector<std::byte> bytes(count);
	if (MPI_Recv(bytes.data(), bytes.size(), MPI_BYTE, 0, MPI_ANY_TAG, MPI_COMM_WORLD, &status) != MPI_SUCCESS) {
		throw std::runtime_error("failed to receive a task");
	}
	std::shared_ptr<node> task{std::make_shared<node>()};
	task->s.deserialize(bytes);
	return task;
}

void send_end_signal(int id) {
	if (MPI_Send(nullptr, 0, MPI_BYTE, id, END_TAG, MPI_COMM_WORLD) != MPI_SUCCESS) {
		throw std::runtime_error("failed to send a end signal");
	}
}

void send_utility(double utility) {
	if (MPI_Send(&utility, 1, MPI_DOUBLE, 0, UTILITY_TAG, MPI_COMM_WORLD) != MPI_SUCCESS) {
		throw std::runtime_error("failed to send a utility");
	}
}

void send_task(const int id, std::shared_ptr<node> task) {
	std::vector<std::byte> bytes;
	task->s.serialize(bytes);
	if (MPI_Send(bytes.data(), bytes.size(), MPI_BYTE, id, TASK_TAG, MPI_COMM_WORLD) != MPI_SUCCESS) {
		throw std::runtime_error("failed to send a task");
	}
}

std::vector<std::shared_ptr<node>> generate_tasks(std::shared_ptr<node> root_node) {
	std::vector<std::shared_ptr<node>> tasks;
	generate_tasks_rec(tasks, root_node);
	return tasks;
}

void generate_tasks_rec(std::vector<std::shared_ptr<node>>& tasks, std::shared_ptr<node> root) {
	if (root->children.empty() && root->s.utility == 0.0) {
		tasks.push_back(root);
		return;
	}
	for (std::shared_ptr<node> subnode : root->children) {
		generate_tasks_rec(tasks, subnode);
	}
}

// Expects a starting state that is not a win.
void build_sched_tree(std::shared_ptr<node> root, uint8_t sched_tree_depth, uint8_t max_depth) {
	const uint8_t depth{static_cast<uint8_t>(max_depth - root->s.remaining_depth)};
	if (depth >= sched_tree_depth) {
		return;
	}
	// Generate subnodes
	for (uint8_t col{0}; col < root->s.b.width; ++col) {
		// Compute the next valid move
		uint8_t h{0};
		while (
			h < root->s.b.height
			&& root->s.b(col, h) != EMPTY
		) h++;
		if (h == root->s.b.height) {
			// The column is full, skip the column.
			continue;
		}

		// Compute the player
		uint8_t next_player{COMPUTER};
		if (root->s.last_move_player == COMPUTER) {
			next_player = HUMAN;
		}

		// Compute the board
		board new_board{root->s.b.copy()};
		new_board(col, h) = next_player;

		// Compute the state
		state new_state(
			new_board,
			root->s.remaining_depth - 1,
			col,
			next_player
		);

		// Check if there's connect 4
		if (new_board.check_connect4(col, h)) {
			if (next_player == COMPUTER) {
				new_state.utility = 1.0;
				// The computer always selects a move (subnode) that wins the game.
				root->s.utility = 1.0;
			} else {
				new_state.utility = -1.0;
				// Human can make a mistake
			}
		}

		std::shared_ptr<node> new_node{std::make_shared<node>(new_state)};
		if (new_state.utility == 1.0) {
			// No need to consider other moves (subnodes). The computer always makes the move that wins the game.
			root->children.clear();
			root->children.push_back(new_node);
			return;
		}

		root->children.push_back(new_node);
		if (new_state.utility == -1.0) {
			// Don't consider subnodes if a human won.
			continue;
		}
		build_sched_tree(new_node, sched_tree_depth, max_depth);
	}
}

state read_state(const std::string filename) {
	std::ifstream file(filename);

	int width, height;

	file >> height >> width;
	board b(width, height);
	for (int j{0}; j < height; ++j) {
		for (int i{0}; i < width; ++i) {
			int field;
			file >> field;
			b(i, height - 1 - j) = static_cast<uint8_t>(field);
		}
	}

	return state(b, 0, 0, HUMAN);
}

