#include <iostream>
#include <cstdlib>
#include <random>
#include <thread>
#include <chrono>
#include <exception>

#include <mpi.h>

// Global constants
static constexpr int FORK_REQUEST{0};
static constexpr int FORK_RESPONSE{1};
static constexpr int LEFT{0};
static constexpr int RIGHT{1};

// Returns a random integer in the specified range.
long rand_between(long min, long max) {
	std::random_device rd;
	std::mt19937 gen(rd());
	std::uniform_int_distribution<long> dist(min, max);
	return dist(gen);
}

inline int fork_global_to_hand(int fork_id, int id, int size) {
	if (fork_id == id) {
		return LEFT;
	} else if (fork_id == (id + 1) % size) {
		return RIGHT;
	} else {
		throw std::invalid_argument("can't map a global fork id to a hand");
	}
	return LEFT;
}

inline int fork_hand_to_global(int hand, int id, int size) {
	if (hand == LEFT) {
		return id;
	} else if (hand == RIGHT) {
		return (id + 1) % size;
	} else {
		throw std::invalid_argument("can't map a hand to a global fork id");
	}
	return 0;
}

inline int neighbor_id(int hand, int id, int size) {
	if (hand == LEFT) {
		return (id - 1 + size) % size;
	} else if (hand == RIGHT) {
		return (id + 1) % size;
	} else {
		throw std::invalid_argument("can't map a hand to a neighbor");
	}
	return 0;
}

inline void output_tabs(int count) {
	for (int i{0}; i < count; ++i) std::cout << '\t';
}

int main(int argc, char* argv[]) {
	MPI_Init(nullptr, nullptr);

	int size, id;
	MPI_Comm_size(MPI_COMM_WORLD, &size);
	MPI_Comm_rank(MPI_COMM_WORLD, &id);

	bool has_fork[]{false, true};
	if (id == 0) has_fork[LEFT] = true;
	if (id == size - 1) has_fork[RIGHT] = false;
	bool is_dirty[]{true, true};
	bool is_requested[]{false, false};
	while (true) {
		// Think
		output_tabs(id); std::cout << "Thinking (" << id << ")" << std::endl;
		const auto deadline{std::chrono::steady_clock::now() + std::chrono::milliseconds(rand_between(200, 3000))};
		while (deadline > std::chrono::steady_clock::now()) {
			// Check for messages
			for (const int hand : {LEFT, RIGHT}) {
				const int other{neighbor_id(hand, id, size)};
				int has_msg{0};
				MPI_Iprobe(other, FORK_REQUEST, MPI_COMM_WORLD, &has_msg, MPI_STATUS_IGNORE);
				if (!has_msg) continue;
				// There is a request, consume it
				int fork_id;
				MPI_Recv(&fork_id, 1, MPI_INT, other, FORK_REQUEST, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
				// Inform the receiver
				MPI_Send(&fork_id, 1, MPI_INT, other, FORK_RESPONSE, MPI_COMM_WORLD);
				// Update our forks
				has_fork[fork_global_to_hand(fork_id, id, size)] = false;
			}
			// Sleep
			std::this_thread::sleep_for(std::chrono::milliseconds(100));
		}

		// Check if we have forks
		while (!has_fork[LEFT] || !has_fork[RIGHT]) {
			// Request a missing fork
			int wanted_fork;
			int wanted_fork_id;
			if (!has_fork[LEFT]) {
				wanted_fork_id = fork_hand_to_global(LEFT, id, size);
				MPI_Send(&wanted_fork_id, 1, MPI_INT, neighbor_id(LEFT, id, size), FORK_REQUEST, MPI_COMM_WORLD);
				wanted_fork = LEFT;
				output_tabs(id); std::cout << "Requesting (" << wanted_fork_id << ")" << std::endl;
			} else {
				wanted_fork_id = fork_hand_to_global(RIGHT, id, size);
				MPI_Send(&wanted_fork_id, 1, MPI_INT, neighbor_id(RIGHT, id, size), FORK_REQUEST, MPI_COMM_WORLD);
				wanted_fork = RIGHT;
				output_tabs(id); std::cout << "Requesting (" << wanted_fork_id << ")" << std::endl;
			}

			// Wait for messages
			do {
				MPI_Status status;
				int fork_id;
				MPI_Recv(&fork_id, 1, MPI_INT, MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &status);
				const int hand{fork_global_to_hand(fork_id, id, size)};
				if (status.MPI_TAG == FORK_RESPONSE) {
					has_fork[hand] = true;
					is_dirty[hand] = false;
					is_requested[hand] = false;
				} else {
					// This is a request for a fork
					if (is_dirty[hand]) {
						// Give it away
						MPI_Send(&fork_id, 1, MPI_INT, status.MPI_SOURCE, FORK_RESPONSE, MPI_COMM_WORLD);
						has_fork[hand] = false;
						is_requested[hand] = false;
					} else {
						// Deny, but save the request
						is_requested[hand] = true;
					}
				}
			} while (!has_fork[wanted_fork]);
		}

		// Eat
		is_dirty[LEFT] = is_dirty[RIGHT] = true;
		output_tabs(id); std::cout << "Eating" << std::endl;

		// Answer the requests
		for (const int fork : {LEFT, RIGHT}) {
			if (!is_requested[fork]) continue;
			const int fork_id{fork_hand_to_global(fork, id, size)};
			MPI_Send(&fork_id, 1, MPI_INT, neighbor_id(fork, id, size), FORK_RESPONSE, MPI_COMM_WORLD);
		}
	}

	MPI_Finalize();
	return EXIT_SUCCESS;
}
