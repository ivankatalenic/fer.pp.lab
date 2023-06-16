#include <iostream>
#include <vector>
#include <memory>
#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <random>
#include <stdexcept>
#include <cmath>

#define check(s) {\
	try {\
		s;\
	} catch (...) {\
		std::cerr << "Exception on line " << __LINE__ << std::endl;\
		throw;\
	}\
}

double avg_dist(std::vector<double> X, std::vector<double> Y) {
	if (X.size() != Y.size()) {
		throw std::invalid_argument("the sizes can't differ");
	}
	const std::size_t N{X.size()};
	const std::size_t distances_cnt{(N * (N - 1)) / 2};
	double avg{0.0};
	for (std::size_t i{0u}; i < N - 1; ++i) {
		for (std::size_t j{i + 1}; j < N; ++j) {
			const double dist{std::sqrt(std::pow(X[i] - X[j], 2) + std::pow(Y[i] - Y[j], 2))};
			avg += dist / distances_cnt;
		}
	}
	return avg;
}

int main(int argc, char* argv[]) {
	if (argc < 2) {
		std::cerr << argv[0] << " <num_of_cities> [<seed>]" << std::endl;
		return -1;
	}
	const int N{atoi(argv[1])};
	int seed{0};
	bool seed_set{false};
	if (argc > 2) {
		seed_set = true;
		seed = atoi(argv[2]);
	}
	
	std::vector<double> X(N, 0.0f);
	std::vector<double> Y(N, 0.0f);

	std::random_device rd;
	std::mt19937 gen(rd());
	if (seed_set) {
		gen = std::mt19937(seed);
	}
	std::uniform_real_distribution<double> dist(0.0, 1.0);
	for (std::size_t i{0u}; i < N; ++i) {
		X[i] = dist(gen);
		Y[i] = dist(gen);
	}

	const double avg{avg_dist(X, Y)};

	std::cout << "Average distance: " << avg << std::endl;

	return 0;
}
