#ifndef user_julia_hpp
#define user_julia_hpp
#include "tbb/parallel_for.h"
#include "puzzler/puzzles/julia.hpp"
#include <string> 


class JuliaProvider
  : public puzzler::JuliaPuzzle
{
public:
  JuliaProvider()
  {}

  virtual void Execute(
		       puzzler::ILog *log,
		       const puzzler::JuliaInput *input,
		       puzzler::JuliaOutput *output
		       ) const override {

    std::vector<unsigned> dest(input->width*input->height);
      
      log->LogInfo("Starting");
      
      // juliaFrameRender_Reference(
      //     input->width,     //! Number of pixels across
      //     input->height,    //! Number of rows of pixels
      //     input->c,        //! Constant to use in z=z^2+c calculation
      //     input->maxIter,   //! When to give up on a pixel
      //     &dest[0]     //! Array of width*height pixels, with pixel (x,y) at pDest[width*y+x]
      // );
      
      log->LogInfo("Mapping");
      
      log->Log(puzzler::Log_Debug, [&](std::ostream &dst){
        dst<<"\n";

        tbb::parallel_for(0u,(unsigned)input->height,[&](unsigned y){
        //for(unsigned y=0;y<input->height;y++){
          for(unsigned x=0;x<input->width;x++){
            unsigned got=dest[y*input->width+x];
            dst<<(got%9);
          }
          dst<<"\n";
        });
      });
      log->LogVerbose("  c = %f,%f,  arg=%f\n", input->c.real(), input->c.imag(), std::arg(input->c));
      
      output->pixels.resize(dest.size());

      //parallel_for slows down for large puzzle sizes here
      //tbb::parallel_for(0u,(unsigned)dest.size(),[&](unsigned i){
      for(unsigned i=0; i<dest.size(); i++){
        output->pixels[i] = (dest[i]==input->maxIter) ? 0 : (1+(dest[i]%256));
      }
      
      log->LogInfo("Finished");

    return ReferenceExecute(log, input, output);
  }

};

#endif
