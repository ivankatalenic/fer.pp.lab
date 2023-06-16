#define CL_HPP_ENABLE_EXCEPTIONS
#define __CL_ENABLE_EXCEPTIONS

#include "CL/cl.hpp"

#include <iostream>
#include <vector>
#include <memory>
#include <algorithm>

#define check_err(s) {\
	try {\
		s;\
	} catch (...) {\
		std::cerr << "Exception on line " << __LINE__ << std::endl;\
		throw;\
	}\
}

int main(int argc, char* argv[]) {

	try {
		std::vector<cl::Platform> platforms;
		cl::Platform::get(&platforms);
		cl::Device device;
		bool is_device_found{false};
		std::cout << "Found platforms:\n";
		for (cl::Platform& p : platforms) {
			std::cout << '\t' << p.getInfo<CL_PLATFORM_NAME>() << std::endl;
			
			const std::string platver{p.getInfo<CL_PLATFORM_VERSION>()};
			if (platver.find("OpenCL 2.") == std::string::npos && platver.find("OpenCL 3.") == std::string::npos) {
				// Skip old platforms
				std::cout << "\t\t" << "skipped the old platform" << std::endl;
				continue;
			}

			std::vector<cl::Device> devices;
			p.getDevices(CL_DEVICE_TYPE_ALL, &devices);
			if (devices.size() == 0) {
				std::cout << "\t\t" << "no devices found on this platform" << std::endl;
				continue;
			}

			for (cl::Device& d : devices) {
				std::cout << "\t\t" << d.getInfo<CL_DEVICE_NAME>() << std::endl;
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
		std::cout << "Selected device: " << device.getInfo<CL_DEVICE_NAME>() << std::endl;

		cl::Context context(device);
		cl::CommandQueue queue(context, device);
		
		std::string source{R"CLC(
			kernel void add(global const float* A, global const float* B, global float* C) {
				int i = get_global_id(0);
				C[i] = A[i] + B[i];
			}
		)CLC"};
		cl::Program program(context, source);
		try {
			check_err(program.build("-cl-std=CL2.0"));
		} catch (...) {
			cl_int build_err{CL_SUCCESS};
			auto build_info{program.getBuildInfo<CL_PROGRAM_BUILD_LOG>(device, &build_err)};
			std::cerr << build_info << std::endl << std::endl;
			return -1;
		}

		constexpr std::size_t N{1024};
		std::vector<float> A(N, 1.0f);
		std::vector<float> B(N, 2.0f);
		std::vector<float> C(N, 0.0f);

		cl::Kernel kernel(program, "add");
		cl::Buffer buf_a(context, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR, N * sizeof(float), A.data());
		cl::Buffer buf_b(context, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR, N * sizeof(float), B.data());
		cl::Buffer buf_c(context, CL_MEM_WRITE_ONLY, N * sizeof(float));
		check_err(kernel.setArg(0, buf_a));
		check_err(kernel.setArg(1, buf_b));
		check_err(kernel.setArg(2, buf_c));

		check_err(queue.enqueueNDRangeKernel(kernel, 0, N));
		check_err(queue.finish());
		check_err(queue.enqueueReadBuffer(buf_c, CL_TRUE, 0, N * sizeof(float), C.data()));


		// Check that data matches the performed computation.
		for (std::size_t i{0u}; i < N; ++i) {
			if (C[i] != 3.0f) {
				std::cout << "The kernel operation on " << i << " saved an invalid result: " << C[i] << std::endl;
				return -1;
			}
		}

	} catch (cl::Error e) {
		std::cerr << e.what() << " (" << e.err() << ")" << std::endl;
		return -1;
	}
	std::cout << "All good!" << std::endl;

	return 0;
}
