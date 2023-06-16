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

#include "jacobi_opencl.hh"

#define check(s) {\
	try {\
		s;\
	} catch (...) {\
		std::cerr << "Exception on line " << __LINE__ << std::endl;\
		throw;\
	}\
}

Jacobi::Jacobi(int mi, int ni, double* resi, double* psii): m{mi}, n{ni}, res{resi}, psi{psii} {
	std::vector<cl::Platform> platforms;
	cl::Platform::get(&platforms);
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
		throw std::runtime_error("No compatible devices found!");
	}
	#ifdef DEBUG
	std::cout << "Selected device: " << device.getInfo<CL_DEVICE_NAME>() << std::endl;
	#endif

	check(context = cl::Context(device));
	check(queue = cl::CommandQueue(context, device));
	
	source = R"CLC(
		kernel void step(global double* psinew, global const double* psi, int m, int n) {
			const int i = get_global_id(0) + 1;
			const int j = get_global_id(1) + 1;
			psinew[i*(m+2)+j] = 0.25 * (psi[(i-1)*(m+2)+j] + psi[(i+1)*(m+2)+j] + psi[i*(m+2)+j-1] + psi[i*(m+2)+j+1]);
		}
	)CLC";
	check(program = cl::Program(context, source));
	try {
		check(program.build("-cl-std=CL2.0"));
	} catch (...) {
		cl_int build_err{CL_SUCCESS};
		auto build_info{program.getBuildInfo<CL_PROGRAM_BUILD_LOG>(device, &build_err)};
		std::cerr << build_info << std::endl << std::endl;
		throw;
	}

	size = (m + 2) * (n + 2) * sizeof(double);
	check(kernel = cl::Kernel(program, "step"));
	check(output = cl::Buffer(context, CL_MEM_WRITE_ONLY, size));
	check(input = cl::Buffer(context, CL_MEM_READ_ONLY | CL_MEM_USE_HOST_PTR, size, psi));
	check(kernel.setArg(0, output));
	check(kernel.setArg(1, input));
	check(kernel.setArg(2, m));
	check(kernel.setArg(3, n));
}

void Jacobi::step() {
	check(queue.enqueueWriteBuffer(input, CL_FALSE, 0, size, psi));
	check(queue.enqueueNDRangeKernel(kernel, cl::NDRange(0, 0), cl::NDRange(m, n)));
	check(queue.finish());
	check(queue.enqueueReadBuffer(output, CL_TRUE, 0, size, res));
}


void jacobistep(double *psinew, double *psi, int m, int n)
{
	int i, j;
  
	for(i=1;i<=m;i++) {
		for(j=1;j<=n;j++) {
		psinew[i*(m+2)+j]=0.25*(psi[(i-1)*(m+2)+j]+psi[(i+1)*(m+2)+j]+psi[i*(m+2)+j-1]+psi[i*(m+2)+j+1]);
		}
	}
}


double deltasq(const std::vector<double>& newarr, const std::vector<double>& oldarr, int m, int n)
{
	int i, j;

	double dsq=0.0;
	double tmp;

	for(i=1;i<=m;i++)
	{
		for(j=1;j<=n;j++)
	{
		tmp = newarr[i*(m+2)+j]-oldarr[i*(m+2)+j];
		dsq += tmp*tmp;
		}
	}

	return dsq;
}
