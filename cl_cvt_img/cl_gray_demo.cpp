#include <opencv2/opencv.hpp>
#include<opencv2/highgui.hpp>
#include <CL/cl.h>
#include <iostream>
#include <fstream>

#define IN
#define OUT
#define ERR_IF(condition,msg) if(condition) { \
								perror(msg); \
								return;}


void load_cl_prog(IN const char* file_path, OUT char* file_buf, OUT size_t & file_size);

void setupCl(unsigned int uiWidth, unsigned int uiHeight, cv::Mat srcImage, cv::Mat grayImg)
{
	cl_int error;
	cl_platform_id platform;
	cl_device_id device;
	cl_context context;
	cl_command_queue cq;
	cl_program program;
	cl_kernel kernel;

	//获取平台id
	error = clGetPlatformIDs(1, &platform, NULL);   
	ERR_IF(CL_SUCCESS != error, "can not get platform ids");

	//获取设备id
	error = clGetDeviceIDs(platform, CL_DEVICE_TYPE_GPU, 1, &device, NULL);
	ERR_IF(CL_SUCCESS != error, "can not get device ids");
	
	//创建上下文
	context = clCreateContext(NULL, 1, &device, NULL, NULL, &error);
	ERR_IF(CL_SUCCESS != error, "create context faild");
	//创建command queue
	cq = clCreateCommandQueue(context, device, NULL, &error);

	//load kernel src
	std::string file_path = "./gray.cl";
	char* file_buf = nullptr;
	size_t file_size = 0;
	load_cl_prog(file_path.c_str(), file_buf, file_size);
	program = clCreateProgramWithSource(context, 1, &file_buf, &file_size, &error);
	ERR_IF(error != CL_SUCCESS, "Load Program Faild")

	//build
	error = clBuildProgram(program, 1, &device, NULL, NULL, NULL);
	if (error != CL_SUCCESS)
	{
		size_t log_info_size = 0;
		clGetProgramBuildInfo(program, device, CL_PROGRAM_BUILD_LOG, 0, NULL, &log_info_size);

		char* log_buf = new char[log_info_size + 1];
		log_buf[log_info_size] = '\0';
		clGetProgramBuildInfo(program, device, CL_PROGRAM_BUILD_LOG, log_info_size + 1, log_buf, NULL);

		printf("\n=== ERROR ===\n\n%s\n=============\n", log_buf);
		delete[] log_buf;
		return;
	}

	//create kernel
	error = clCreateKernelsInProgram(program, 1, &kernel, NULL);
	ERR_IF(error != CL_SUCCESS, "create kernel failed");

	cl_mem memRgbImg = clCreateBuffer(context, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR, \
		sizeof(uchar) * 3 * uiWidth * uiHeight, srcImage.data, &error);

	cl_mem memGrayImg = clCreateBuffer(context, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR, \
		sizeof(uchar) * uiWidth * uiHeight, srcImage.data, &error);

	cl_mem memImgHeight = clCreateBuffer(context, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR, \
		sizeof(int), &uiHeight, &error);

	cl_mem memImgWidth = clCreateBuffer(context, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR, \
		sizeof(int), &uiWidth, &error);

	//向内核传递参数
	error = clSetKernelArg(kernel, 0, sizeof(cl_mem), &memRgbImg);
	error = clSetKernelArg(kernel, 1, sizeof(cl_mem), &memGrayImg);
	error = clSetKernelArg(kernel, 2, sizeof(cl_mem), &memImgHeight);
	error = clSetKernelArg(kernel, 3, sizeof(cl_mem), &memImgWidth);

	size_t localThreads[2] = { 32, 4 };   //工作组中工作项的排布
	size_t globalThreads[2] = { ((uiWidth + localThreads[0] - 1) / localThreads[0]) * localThreads[0],
								((uiHeight + localThreads[1] - 1) / localThreads[1]) * localThreads[1] };   //整体排布

	cl_event evt;
	error = clEnqueueNDRangeKernel(cq, kernel, 2, 0, globalThreads, localThreads, 0, NULL, &evt);
	clWaitForEvents(1, &evt);
	clReleaseEvent(evt);

	error = clEnqueueReadBuffer(cq, memGrayImg, CL_TRUE, 0, sizeof(uchar) * uiWidth * uiHeight, grayImg.data, 0, NULL, NULL);
	cv::imshow("srcimg", srcImage);
	cv::imshow("grayimg", grayImg);
	cv::waitKey(0);

	clReleaseMemObject(memRgbImg);
	clReleaseMemObject(memGrayImg);
	clReleaseMemObject(memImgHeight);
	clReleaseMemObject(memImgWidth);
	clReleaseKernel(kernel);
	clReleaseProgram(program);
	clReleaseCommandQueue(cq);
	clReleaseContext(context);

	delete[] file_buf;
	delete& srcImage;
	delete& grayImg;

	return;

}

void load_cl_prog(IN const char* file_path, OUT char* file_buf, OUT size_t& file_size)
{
	std::ifstream cl_file;
	cl_file.open(file_path, std::ios::out);

	ERR_IF(!cl_file.is_open(), "CL Prog File Open Failed");
	cl_file.seekg(std::ios::end);
	file_size = cl_file.tellg();
	cl_file.seekg(std::ios::beg);

	file_buf = new char[file_size];
	cl_file.read(file_buf, file_size);
	cl_file.close();
}
