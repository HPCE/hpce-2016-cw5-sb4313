#ifndef user_random_walk_hpp
#define user_random_walk_hpp

#include "puzzler/puzzles/random_walk.hpp"
#include "tbb/parallel_for.h"


class RandomWalkProvider
: public puzzler::RandomWalkPuzzle
{
public:
  RandomWalkProvider()
  {}

private:

  uint32_t step(uint32_t x) const
  {
    return x*1664525+1013904223;
  };

protected:
    /* Start from node start, then follow a random walk of length nodes, incrementing
       the count of all the nodes we visit. */
  void random_walk(std::vector<puzzler::dd_node_t> &nodes, std::vector<tbb::atomic<uint32_t>> &atomic_count, uint32_t seed, unsigned start, unsigned length) const
  {
    uint32_t rng=seed;
    unsigned current=start;

    for(unsigned i=0; i<length; i++){

      atomic_count[current]++;

      unsigned edgeIndex = rng % nodes[current].edges.size();
      rng=rng*1664525+1013904223; 

      current=nodes[current].edges[edgeIndex];
    }
  }

  virtual void Execute(
   puzzler::ILog *log,
   const puzzler::RandomWalkInput *input,
   puzzler::RandomWalkOutput *output
   ) const override {

      // Take a copy, as we'll need to modify the "count" flags
    std::vector<puzzler::dd_node_t> nodes(input->nodes);
    std::vector<tbb::atomic<uint32_t>> atomic_count(nodes.size(), 0);

    log->Log(puzzler::Log_Debug, [&](std::ostream &dst){
      dst<<"  Scale = "<<nodes.size()<<"\n";
      for(unsigned i=0;i<nodes.size();i++){
        dst<<"  "<<i<<" -> [";
          for(unsigned j=0;j<nodes[i].edges.size();j++){
            if(j!=0)
              dst<<",";
            dst<<nodes[i].edges[j];
          }
          dst<<"]\n";
  }
});

    log->LogVerbose("Starting random walks");

    int choose = 1; // opencl version failed at the end

    if(choose == 1)
    {
      // This gives the same sequence on all platforms
      std::mt19937 rng(input->seed);

      std::vector<unsigned> seed;
      std::vector<unsigned> start;

      for(int i = 0; i<input->numSamples; i++)
      {
        seed.push_back(rng());
        start.push_back(rng() % nodes.size());    // Choose a random node
      }

    unsigned length=input->lengthWalks;           // All paths the same length

    tbb::parallel_for(0u, (unsigned)input->numSamples,[&](unsigned i){
    //for(unsigned i=0; i<input->numSamples; i++){
        //unsigned seed=rng();
        //unsigned start=rng() % nodes.size();    // Choose a random node

      random_walk(nodes,atomic_count, seed[i], start[i], length);
    });

    log->LogVerbose("Done random walks, converting histogram");
  }


 /* if(choose == 2)
  {
    std::vector<uint32_t> edges;
    for(int i = 0; i < input->nodes[0].edges.size(); i++)
    {
      edges.insert(edges.end(),input->nodes[i].begin(),input->nodes[i].end());
    }

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

    std::string kernelSource=LoadSource("user_random_walk.cl");


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

        cl::Kernel kernel(program, "random_walk_kernel");

        cl::CommandQueue queue(context, device);

        size_t buffer_edge = edges.size();
        size_t buffer_seed = seed.size();
        size_t buffer_start = start.size();
        size_t buffer_count = atomic_count.size();

        cl::Buffer buffEdge(context, CL_MEM_READ_ONLY, buffer_edge);
        cl::Buffer buffSeed(context, CL_MEM_READ_ONLY, buffer_seed);
        cl::Buffer buffStart(context, CL_MEM_READ_ONLY, buffer_start);
        cl::Buffer buffCount(context, CL_MEM_WRITE_ONLY, buffer_count);

        queue.enqueueWriteBuffer(buffEdge, CL_FALSE,0,buffer_edge,&edge[0]);
        queue.enqueueWriteBuffer(buffSeed, CL_FALSE,0,buffer_seed,&seed[0]);
        queue.enqueueWriteBuffer(buffStart, CL_FALSE,0,buffer_start,&start[0]);
        queue.enqueueWriteBuffer(buffCount, CL_FALSE,0,buffer_seed,&seed[0]);

        //...

        queue.enqueueBarrier();
        
        kernel.setArg(0, buffEdge); // done
        kernel.setArg(1, buffCount); 
        kernel.setArg(2, length); // done
        kernel.setArg(3, buffSeed); // done
        kernel.setArg(4, buffStart); // done 
        kernel.setArg(5, edges.size()); // done


        cl::Event evCopiedState;
            //queue.enqueueWriteBuffer(buffState, CL_FALSE, 0, cbBuffer, &world.state[0], NULL, &evCopiedState);

        cl::NDRange offset(0, 0);               // Always start iterations at x=0, y=0
        cl::NDRange globalSize(length);   // Global size must match the original loops
        cl::NDRange localSize=cl::NullRange;    // We don't care about local size

        std::vector<cl::Event> kernelDependencies(1, evCopiedState);

        cl::Event evExecutedKernel;

        queue.enqueueNDRangeKernel(kernel, offset, globalSize, localSize, NULL, &evExecutedKernel);

        //queue.enqueueBarrier();

        std::vector<cl::Event> copyBackDependencies(1, evExecutedKernel);

        queue.enqueueReadBuffer(buffCount, CL_TRUE, 0, buffer_count, &atomic_count[0], &copyBackDependencies);

      }
    }*/

      // Map the counts from the nodes back into an array
    output->histogram.resize(nodes.size());
    for(unsigned i=0; i<nodes.size(); i++){
      output->histogram[i]=std::make_pair(uint32_t(atomic_count[i]),uint32_t(i));
      atomic_count[i]=0;
    }
      // Order them by how often they were visited
    std::sort(output->histogram.rbegin(), output->histogram.rend());


      // Debug only. No cost in normal execution
    log->Log(puzzler::Log_Debug, [&](std::ostream &dst){
      for(unsigned i=0; i<output->histogram.size(); i++){
        dst<<"  "<<i<<" : "<<output->histogram[i].first<<", "<<output->histogram[i].second<<"\n";
      }
    });

    log->LogVerbose("Finished");
}

  };

#endif

/*tbb::parallel_for(0u,(unsigned)length,[&](unsigned i){
        nodes[current].count++;

        unsigned edgeIndex = rng % nodes[current].edges.size();
        rng=step(rng);
        
        current=nodes[current].edges[edgeIndex];
      });
*/
