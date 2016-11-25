#ifndef user_julia_hpp
#define user_julia_hpp

#include <random>
#include <sstream>
#include <algorithm>
#include <complex>
#include <memory>

#include "tbb/parallel_for.h"

#include "puzzler/puzzles/julia.hpp"

#define __CL_ENABLE_EXCEPTIONS 
#include "CL/cl.hpp"
#include <fstream>
#include <streambuf>

class JuliaProvider
: public puzzler::JuliaPuzzle
{
public:
  JuliaProvider()
  {}

  void juliaFrameRender_Reference(
      unsigned width,     //! Number of pixels across
      unsigned height,    //! Number of rows of pixels
      puzzler::complex_t c,        //! Constant to use in z=z^2+c calculation
      unsigned maxIter,   //! When to give up on a pixel
      unsigned *pDest     //! Array of width*height pixels, with pixel (x,y) at pDest[width*y+x]
      ) const
  {

    float dx=3.0f/width, dy=3.0f/height;
    //unsigned useKernel = 1;

  if(maxIter < 900) // change to max_iter <1000
  {
    unsigned inner_K = getLoopK();

    typedef tbb::blocked_range<unsigned> my_range_t;

    my_range_t range(0,height,inner_K);

    auto f = [=](const my_range_t &chunk)
    {
      for(unsigned y = chunk.begin(); y!=chunk.end();y++){
        for(unsigned x=0; x<width; x++) {
              // Map pixel to z_0
          puzzler::complex_t z(-1.5f+x*dx, -1.5f+y*dy);

              //Perform a julia iteration starting at the point z_0, for offset c.
              //   z_{i+1} = z_{i}^2 + c
              // The point escapes for the first i where |z_{i}| > 2.
          unsigned iter=0;
          while(iter<maxIter){
            if(abs(z) > 2){
              break;
            }
                  // Anybody want to refine/tighten this?
            z = z*z + c;
            ++iter;
          }
          pDest[y*width+x] = iter;
        }
      }
    };

    tbb::parallel_for (range, f, tbb::simple_partitioner());
  }

  if(maxIter >= 900)
  {
    // Select platform
    std::vector<cl::Platform> platforms;

    cl::Platform::get(&platforms);
    if(platforms.size()==0)
      throw std::runtime_error("No OpenCL platforms found.");

    std::cerr<<"Found "<<platforms.size()<<" platforms\n";
    for(unsigned i=0;i<platforms.size();i++){
      std::string vendor=platforms[i].getInfo<CL_PLATFORM_VENDOR>();
      std::cerr<<"  Platform "<<i<<" : "<<vendor<<"\n";
    }

    int selectedPlatform=0;
    if(getenv("HPCE_SELECT_PLATFORM")){
      selectedPlatform=atoi(getenv("HPCE_SELECT_PLATFORM"));
    }
    std::cerr<<"Choosing platform "<<selectedPlatform<<"\n";
    cl::Platform platform=platforms.at(selectedPlatform);    

        // Select device
    std::vector<cl::Device> devices;
    platform.getDevices(CL_DEVICE_TYPE_ALL, &devices);  
    if(devices.size()==0){
      throw std::runtime_error("No opencl devices found.\n");
    }

    std::cerr<<"Found "<<devices.size()<<" devices\n";
    for(unsigned i=0;i<devices.size();i++){
      std::string name=devices[i].getInfo<CL_DEVICE_NAME>();
      std::cerr<<"  Device "<<i<<" : "<<name<<"\n";
    }

    int selectedDevice=0;
    if(getenv("HPCE_SELECT_DEVICE")){
      selectedDevice=atoi(getenv("HPCE_SELECT_DEVICE"));
    }
    std::cerr<<"Choosing device "<<selectedDevice<<"\n";
    cl::Device device=devices.at(selectedDevice);

    cl::Context context(devices);

    std::string kernelSource=LoadSource("user_julia.cl");

        cl::Program::Sources sources;   // A vector of (data,length) pairs
        sources.push_back(std::make_pair(kernelSource.c_str(), kernelSource.size()+1)); // push on our single string

        cl::Program program(context, sources);
        try{
          program.build(devices);
        }catch(...){
          for(unsigned i=0;i<devices.size();i++){
            std::cerr<<"Log for device "<<devices[i].getInfo<CL_DEVICE_NAME>()<<":\n\n";
            std::cerr<<program.getBuildInfo<CL_PROGRAM_BUILD_LOG>(devices[i])<<"\n\n";
          }
          throw;
        }

        size_t cbBuffer=width * height * 4;
        //cl::Buffer buffProperties(context, CL_MEM_READ_ONLY, cbBuffer);
        //cl::Buffer buffState(context, CL_MEM_READ_ONLY, cbBuffer);
        cl::Buffer buffBuffer(context, CL_MEM_WRITE_ONLY, cbBuffer);

        cl::Kernel kernel(program, "julia_kernel");

        //std::cerr<<"1 "<<"\n";

        //unsigned w=world.w, h=world.h;
        
        //float outer=world.alpha*dt;     // We spread alpha to other cells per time
        //float inner=1-outer/4;              // Anything that doesn't spread stays
        
        kernel.setArg(0, height);
        kernel.setArg(1, width);
        kernel.setArg(2, maxIter);
        kernel.setArg(3, c);
        kernel.setArg(4, buffBuffer);

        cl::CommandQueue queue(context, device);

        //queue.enqueueWriteBuffer(buffProperties, CL_TRUE, 0, cbBuffer, &world.properties[0]);

        // This is our temporary working space
        std::vector<float> buffer(width * height * 4);


        cl::Event evCopiedState;
            //queue.enqueueWriteBuffer(buffState, CL_FALSE, 0, cbBuffer, &world.state[0], NULL, &evCopiedState);

        cl::NDRange offset(0, 0);               // Always start iterations at x=0, y=0
        cl::NDRange globalSize(width, height);   // Global size must match the original loops
        cl::NDRange localSize=cl::NullRange;    // We don't care about local size

        std::vector<cl::Event> kernelDependencies(1, evCopiedState);

        cl::Event evExecutedKernel;

        queue.enqueueNDRangeKernel(kernel, offset, globalSize, localSize, NULL, &evExecutedKernel);

        std::vector<cl::Event> copyBackDependencies(1, evExecutedKernel);

        queue.enqueueReadBuffer(buffBuffer, CL_TRUE, 0, cbBuffer, &pDest[0], &copyBackDependencies);

            // All cells have now been calculated and placed in buffer, so we replace
            // the old state with the new state
         //std::swap(pDest, buffer);
      }
    }

    virtual void Execute(
     puzzler::ILog *log,
     const puzzler::JuliaInput *pInput,
     puzzler::JuliaOutput *pOutput
     ) const override {
      std::vector<unsigned> dest(pInput->width*pInput->height);

      std::cout << "timing execution now" << std::endl;
      clock_t startTime = clock();

      log->LogInfo("Starting");

      juliaFrameRender_Reference(
        pInput->width,     //! Number of pixels across
        pInput->height,    //! Number of rows of pixels
        pInput->c,        //! Constant to use in z=z^2+c calculation
        pInput->maxIter,   //! When to give up on a pixel
        &dest[0]     //! Array of width*height pixels, with pixel (x,y) at pDest[width*y+x]
        );

      log->LogInfo("Mapping");

      log->Log(puzzler::Log_Debug, [&](std::ostream &dst){
        dst<<"\n";
        for(unsigned y=0;y<pInput->height;y++){
          for(unsigned x=0;x<pInput->width;x++){
            unsigned got=dest[y*pInput->width+x];
            dst<<(got%9);
          }
          dst<<"\n";
        }
      });
      log->LogVerbose("  c = %f,%f,  arg=%f\n", pInput->c.real(), pInput->c.imag(), std::arg(pInput->c));

      pOutput->pixels.resize(dest.size());
      for(unsigned i=0; i<dest.size(); i++){
        pOutput->pixels[i] = (dest[i]==pInput->maxIter) ? 0 : (1+(dest[i]%256));
      }

      log->LogInfo("Finished");

      std::cout << "This took " << double(clock() - startTime) / (double)CLOCKS_PER_SEC << " seconds" << std::endl;

    }

    virtual unsigned getLoopK(void) const
    {

      char *v=getenv("HPCE_FFT_LOOP_K");
      if(v==NULL)
      {
       return 16; 
     }
     else
     {
       return atoi(v);
     }
   }

   virtual unsigned getKernel(void) const
   {

    char *v=getenv("GET_KERNEL");
    if(v==NULL)
    {
     return 1; 
   }
   else
   {
     return atoi(v);
   }
 }

 std::string LoadSource(const char *fileName) const
 {
  std::string baseDir="provider";
  if(getenv("HPCE_CL_SRC_DIR")){
    baseDir=getenv("HPCE_CL_SRC_DIR");
  }

  std::string fullName=baseDir+"/"+fileName;

  std::ifstream src(fullName, std::ios::in | std::ios::binary);
  if(!src.is_open())
    throw std::runtime_error("LoadSource : Couldn't load cl file from '"+fullName+"'.");

  return std::string(
          (std::istreambuf_iterator<char>(src)), // Node the extra brackets.
          std::istreambuf_iterator<char>()
          );
}

};


#endif