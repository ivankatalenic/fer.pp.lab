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
#include <iomanip>

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
		std::cerr << argv[0] << " <work_size> <work_per_thread> [<work_group_size>]" << std::endl;
		return -1;
	}
	uint64_t work_size{static_cast<uint64_t>(std::strtoull(argv[1], nullptr, 10))};
	bool is_work_per_thread_set{false};
	uint64_t work_per_thread;
	if (argc >= 3) {
		is_work_per_thread_set = true;
		work_per_thread = static_cast<uint64_t>(std::strtoull(argv[2], nullptr, 10));
	}
	bool is_work_group_size_set{false};
	uint64_t work_group_size;
	if (argc >= 4) {
		is_work_group_size_set = true;
		work_group_size = static_cast<uint64_t>(std::strtoull(argv[3], nullptr, 10));
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
			kernel void sum(const unsigned long work_size, const unsigned long work_per_thread, global double* results) {
				const size_t thread_id = get_global_id(0);
				
				const size_t start = thread_id * work_per_thread + 1;
				const size_t end   = (thread_id + 1) * work_per_thread;
				
				results[thread_id] = 0.0;

				const double n = (double) work_size;
				const double nr = 1.0 / n;

				for (size_t i = start; i <= end && i <= work_size; i++) {
					const double p = nr * (i - 0.5);
					const double res = 4.0 / (n * (1.0 + pown(p, 2)));
					results[thread_id] += res;
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


		const uint64_t threads_cnt{(work_size + work_per_thread - 1) / work_per_thread};

		std::vector<double> results(threads_cnt, 0.0);

		cl::Kernel kernel(program, "sum");
		cl::Buffer buf_results(context, CL_MEM_WRITE_ONLY, threads_cnt * sizeof(double));
		check(kernel.setArg(0, work_size));
		check(kernel.setArg(1, work_per_thread));
		check(kernel.setArg(2, buf_results));

		if (is_work_group_size_set) {
			check(queue.enqueueNDRangeKernel(kernel, 0, threads_cnt, work_group_size));
		} else {
			check(queue.enqueueNDRangeKernel(kernel, 0, threads_cnt));
		}
		check(queue.finish());
		check(queue.enqueueReadBuffer(buf_results, CL_TRUE, 0, threads_cnt * sizeof(double), results.data()));

		double pi{0.0};
		for (double result : results) {
			pi += result;
		}

		const double PI25DT{3.141592653589793238462643};
		std::cout << "Approximated Pi: " << std::setprecision(16) << pi << std::endl;
		std::cout << "Error: " << fabs(pi - PI25DT) << std::endl;

	} catch (cl::Error e) {
		std::cerr << e.what() << " (" << e.err() << ")" << std::endl;
		return -1;
	}

	return 0;
}
