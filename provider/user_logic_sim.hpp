#ifndef user_logic_sim_hpp
#define user_logic_sim_hpp

#include "puzzler/puzzles/logic_sim.hpp"
#include <stack>  

class LogicSimProvider
  : public puzzler::LogicSimPuzzle
{
public:
  LogicSimProvider()
  {}

  mutable unsigned chunk_var = 0;

    virtual size_t variable_chunk(void) const
    {
      if(chunk_var != 0){
        return chunk_var;
      }
      char *v= getenv("HPCE_LOGIC_SIM");
      if(v == NULL){
        //printf("HPCE_DIRECT_INNER_K is not set.\n");
        chunk_var = 64;
      }else{
        //printf("HPCE_DIRECT_INNER_K = %s\n", v);
        chunk_var = atoi(v);
      }
      return chunk_var;
    }




      std::vector<double> next(const std::vector<double> &state, const puzzler::LogicSimInput *input) const
      {

        std::vector<double> res(state.size());
        unsigned K = variable_chunk();
        typedef tbb::blocked_range<unsigned> my_range_t;
        my_range_t range(0,res.size(),K);

        auto f=[&](const my_range_t &chunk){
        for(unsigned i = chunk.begin(); i!= chunk.end();i++){
          res[i]=calcSrc(input->flipFlopInputs[i], state, input);
        }
      };
        tbb::parallel_for(range, f, tbb::simple_partitioner());
        return res;
      }

      double calcSrc(unsigned src, const std::vector<double> &state, const puzzler::LogicSimInput *input) const
      {
        //std::stack<unsigned> srcstack;
        //double a = 0;
        //double done = 0;
        if(src < state.size()){
          return state.at(src);
        }else{
          unsigned xorSrc=src - state.size();
          double a , b;
          a=calcSrc(input->xorGateInputs.at(xorSrc).first, state, input);
          b=calcSrc(input->xorGateInputs.at(xorSrc).second, state, input);
          return a != b;
        }

          /*while(!done){
            if(src>state.size()){
              srcstack.push(src);
              src = input->xorGateInputs.at(src - state.size()).first;
            }else{
              a = a != state.at(src);
              if(!srcstack.empty()){
                src = srcstack.top();
                srcstack.pop();
                src = input->xorGateInputs.at(src-state.size()).second;
              }
              else{
                done = 1;
              }
            }
          }
          return a;*/
      }

  virtual void Execute(
		       puzzler::ILog *log,
		       const puzzler::LogicSimInput *pInput,
		       puzzler::LogicSimOutput *pOutput
            		       ) const override     {
                          //std::cout << "timing execution" << std::endl;
                          //clock_t startTime = clock();
                          log->LogVerbose("About to start running clock cycles (total = %d", pInput->clockCycles);
                          std::vector<bool> state=pInput->inputState;
                          std::vector<double> vec(state.begin(),state.end());

                          for(unsigned i=0; i<pInput->clockCycles; i++){
                            log->LogVerbose("Starting iteration %d of %d\n", i, pInput->clockCycles);

                            vec=next(vec, pInput);

                            // The weird form of log is so that there is little overhead
                            // if logging is disabled
                            log->Log(puzzler::Log_Debug,[&](std::ostream &dst) {
                              for(unsigned i=0; i<vec.size(); i++){
                              dst<<vec[i];
                              }
                            });
                          }

                          log->LogVerbose("Finished clock cycles");


                          std::vector<bool> out(vec.begin(), vec.end());
                          pOutput->outputState=out;
                          //std::cout << "This took " << double( clock() - startTime ) / (double)CLOCKS_PER_SEC<< " seconds." << std::endl;
                        }

};

#endif
