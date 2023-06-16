//#include <nvtx3/nvToolsExt.h>

#include <cstdio>
#include "CL/cl.hpp"

class Jacobi {
private:
	int m;
	int n;
	double* res;
	double* psi;
	std::size_t size;
	cl::Device device;
	cl::Context context;
	std::string source;
	cl::Program program;
	cl::CommandQueue queue;
	cl::Kernel kernel;
	cl::Buffer output;
	cl::Buffer input;
public:
	Jacobi(int m, int n, double* res, double* psi);
	Jacobi() = default;
	void step();
};

double deltasq(const std::vector<double>& newarr, const std::vector<double>& oldarr, int m, int n);
