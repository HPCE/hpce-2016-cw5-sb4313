#ifndef user_julia_hpp
#define user_julia_hpp

#include <random>
#include <sstream>
#include <algorithm>
#include <complex>


#include "puzzler/puzzles/julia.hpp"

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

        for(unsigned y=0; y<height; y++){
            for(unsigned x=0; x<width; x++){
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
    }

  virtual void Execute(
		       puzzler::ILog *log,
		       const puzzler::JuliaInput *pInput,
		       puzzler::JuliaOutput *pOutput
		       ) const override {
      std::vector<unsigned> dest(pInput->width*pInput->height);
      
      log->LogInfo("Starting");
      
      return juliaFrameRender_Reference(
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
    }

};

#endif
