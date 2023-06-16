#include <iostream>
#include <vector>
#include <memory>
#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <random>
#include <stdexcept>
#include <cmath>
#include <iomanip>

int main(int argc, char* argv[]) {
	if (argc < 2) {
		std::cerr << argv[0] << " <work_size>" << std::endl;
		return -1;
	}
	const uint64_t work_size{static_cast<uint64_t>(std::strtoull(argv[1], nullptr, 10))};

	const double n{static_cast<double>(work_size)};
	double pi{0.0};
	for (uint64_t i{1}; i <= work_size; ++i) {
		const double p{(i - 0.5) / n};
		const double under{n * (1.0 + pow(p, 2.0))};
		pi += 4.0 / under;
	}

	const double PI25DT{3.141592653589793238462643};

	std::cout << "Approximated Pi: " << std::setprecision(16) << pi << std::endl;
	std::cout << "Error: " << fabs(pi - PI25DT) << std::endl;

	return 0;
}
