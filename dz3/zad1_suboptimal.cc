#define CL_HPP_ENABLE_EXCEPTIONS
#define __CL_ENABLE_EXCEPTIONS

#include "CL/cl.hpp"

#include <iostream>
#include <vector>
#include <memory>
#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <random>
#include <stdexcept>
#include <cmath>
#include <cstdint>

#define check(s) {\
	try {\
		s;\
	} catch (...) {\
		std::cerr << "Exception on line " << __LINE__ << std::endl;\
		throw;\
	}\
}

int main(int argc, char* argv[]) {
	if (argc < 3) {
		std::cerr << argv[0] << " <num_of_cities> <work_per_thread> [<seed>]" << std::endl;
		return -1;
	}
	const uint64_t work_size{std::strtoull(argv[1], nullptr, 10)};
	const uint64_t work_per_thread{std::strtoull(argv[2], nullptr, 10)};
	uint64_t seed{0};
	bool seed_set{false};
	if (argc > 3) {
		seed_set = true;
		seed = std::strtoull(argv[3], nullptr, 10);
	}

	try {
		std::vector<cl::Platform> platforms;
		cl::Platform::get(&platforms);
		cl::Device device;
		bool is_device_found{false};
		#ifdef DEBUG
		std::cout << "Found platforms:\n";
		#endif
		for (cl::Platform& p : platforms) {
			#ifdef DEBUG
			std::cout << '\t' << p.getInfo<CL_PLATFORM_NAME>() << std::endl;
			#endif
			
			const std::string platver{p.getInfo<CL_PLATFORM_VERSION>()};
			if (platver.find("OpenCL 2.") == std::string::npos && platver.find("OpenCL 3.") == std::string::npos) {
				// Skip old platforms
				#ifdef DEBUG
				std::cout << "\t\t" << "skipped the old platform" << std::endl;
				#endif
				continue;
			}

			std::vector<cl::Device> devices;
			p.getDevices(CL_DEVICE_TYPE_ALL, &devices);
			if (devices.size() == 0) {
				#ifdef DEBUG
				std::cout << "\t\t" << "no devices found on this platform" << std::endl;
				#endif
				continue;
			}

			for (cl::Device& d : devices) {
				#ifdef DEBUG
				std::cout << "\t\t" << d.getInfo<CL_DEVICE_NAME>() << std::endl;
				#endif
				if (!is_device_found) {
					device = d;
					is_device_found = true;
					continue;
				}
				if (d.getInfo<CL_DEVICE_TYPE>() == CL_DEVICE_TYPE_GPU) {
					device = d;
					is_device_found = true;
					continue;
				}
			}
		}
		if (!is_device_found) {
			std::cout << "No compatible devices found!" << std::endl;
			return -1;
		}
		#ifdef DEBUG
		std::cout << "Selected device: " << device.getInfo<CL_DEVICE_NAME>() << std::endl;
		#endif

		cl::Context context(device);
		cl::CommandQueue queue(context, device);
		
		std::string source{R"CLC(
			kernel void sum(
					global const double* X, global const double* Y, global double* C,
					const int work_size, const int work_per_thread
				) {
				const int thread_id = get_global_id(0);

				const int threads_cnt = work_size / work_per_thread;
				const int remaining_work = work_size - work_per_thread * threads_cnt;

				if (remaining_work > 0 && thread_id == 0) {
					// Do the last remaining items only on the first thread
					for (int r = 0; r < remaining_work; r++) {
						const int start = threads_cnt * work_per_thread + r;
						C[start] = 0.0;
						for (int end = start + 1; end < work_size; end++) {
							C[start] += distance((double2)(X[start], Y[start]), (double2)(X[end], Y[end]));
						}
					}
				}

				for (int i = 0; i < work_per_thread; i++) {
					const int start = thread_id * work_per_thread + i;
					C[start] = 0.0;
					for (int end = start + 1; end < work_size; end++) {
						C[start] += distance((double2)(X[start], Y[start]), (double2)(X[end], Y[end]));
					}
				}
			}
		)CLC"};
		cl::Program program(context, source);
		try {
			check(program.build("-cl-std=CL2.0"));
		} catch (...) {
			cl_int build_err{CL_SUCCESS};
			auto build_info{program.getBuildInfo<CL_PROGRAM_BUILD_LOG>(device, &build_err)};
			std::cerr << build_info << std::endl << std::endl;
			return -1;
		}

		std::vector<double> X(work_size, 0.0f);
		std::vector<double> Y(work_size, 0.0f);
		std::vector<double> R(work_size, 0.0f);

		std::random_device rd;
		std::mt19937 gen(rd());
		if (seed_set) {
			gen = std::mt19937(seed);
		}
		std::uniform_real_distribution<double> dist(0.0, 1.0);
		for (std::size_t i{0u}; i < work_size; ++i) {
			X[i] = dist(gen);
			Y[i] = dist(gen);
		}

		const uint64_t threads_cnt{work_size / work_per_thread};
		cl::Kernel kernel(program, "sum");
		cl::Buffer buf_a(context, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR, work_size * sizeof(double), X.data());
		cl::Buffer buf_b(context, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR, work_size * sizeof(double), Y.data());
		cl::Buffer buf_c(context, CL_MEM_WRITE_ONLY, work_size * sizeof(double));
		check(kernel.setArg(0, buf_a));
		check(kernel.setArg(1, buf_b));
		check(kernel.setArg(2, buf_c));
		check(kernel.setArg(3, static_cast<int>(work_size)));
		check(kernel.setArg(4, static_cast<int>(work_per_thread)));

		check(queue.enqueueNDRangeKernel(kernel, 0, threads_cnt));
		check(queue.finish());
		check(queue.enqueueReadBuffer(buf_c, CL_TRUE, 0, work_size * sizeof(double), R.data()));

		double avg{0.0};
		double total_distances{(work_size * (work_size - 1)) / 2.0};
		for (std::size_t i{0u}; i < work_size; ++i) {
			avg += R[i] / total_distances;
		}
		std::cout << "Average distance: " << avg << std::endl;

	} catch (cl::Error e) {
		std::cerr << e.what() << " (" << e.err() << ")" << std::endl;
		return -1;
	}

	return 0;
}
