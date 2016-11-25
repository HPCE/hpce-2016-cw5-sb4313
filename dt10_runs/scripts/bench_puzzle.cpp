
#include "puzzler/puzzler.hpp"

#include <iostream>
#include <fstream>

#include <cmath>

#include "file_log.hpp"


int main(int argc, char *argv[])
{
   puzzler::PuzzleRegistrar::UserRegisterPuzzles();

   if(argc<2){
      fprintf(stderr, "bench_puzzle name xScale iStart iExp sysLogLevel userLogLevel userLogFile outputCsv budget\n");
      std::cout<<"Puzzles:\n";
      puzzler::PuzzleRegistrar::ListPuzzles();
      exit(1);
   }

   try{
      std::string name=argv[1];

      double xScale=strtod(argv[2],NULL);
      int iStart=atoi(argv[3]);
      double iExp=strtod(argv[4],NULL);
      fprintf(stderr, "n = %f * (%d:1:+inf).^%f\n", xScale, iStart, iExp);

      int sysLogLevel=atoi(argv[5]);
      fprintf(stderr, "SysLogLevel = %s -> %d\n", argv[5], sysLogLevel);

      int userLogLevel=atoi(argv[6]);
      fprintf(stderr, "UserLogLevel = %s -> %d\n", argv[6], userLogLevel);

      std::string userLogFileName=argv[7];

      std::string outputCsvName=argv[8];

      double budget=strtod(argv[9],NULL);

      std::shared_ptr<puzzler::ILog> logSysDest=std::make_shared<puzzler::LogDest>("bench_puzzle/sys", sysLogLevel);
      logSysDest->Log(puzzler::Log_Info, "Created system console log at logLevel=%d.", sysLogLevel);

      std::shared_ptr<puzzler::ILog> logUserDest=std::make_shared<puzzler::LogDest>("bench_puzzle/user", userLogLevel);
      logSysDest->Log(puzzler::Log_Info, "Created user console log at logLevel=%d.", userLogLevel);

      std::shared_ptr<puzzler::ILog> logUserFile=std::make_shared<puzzler::FileLogDest>(userLogFileName, "bench_puzzle/user", userLogLevel+2, logUserDest);
      logSysDest->Log(puzzler::Log_Info, "Created user file log at file '%s' with logLevel=%d.", userLogFileName.c_str(), userLogLevel+2);

      auto puzzle=puzzler::PuzzleRegistrar::Lookup(name);
      if(!puzzle)
         throw std::runtime_error("No puzzle registered with name "+name);

      std::ofstream csvFile(outputCsvName.c_str());
      if(!csvFile.is_open())
         throw std::runtime_error("Couldn't open csv dest file '"+outputCsvName+"'");

      double startTime=puzzler::now()*1e-9;
      double finishTime=startTime+budget;

      logSysDest->LogInfo("Total time budget = %f, start=%f, deadline=%f\n", budget, startTime, finishTime);

      int iCurr=1;
      while(1){
         double t=puzzler::now()*1e-9;
         logSysDest->LogInfo("Time budget used = %f, remaining = %f", t-startTime, finishTime-t);

         if(t >= finishTime)
            break;

         double fn=(xScale*pow(iCurr+iStart,iExp));
         if(fn > pow(2.0,31)-1){
            logSysDest->LogFatal("Calculated scale has exceded 2^31-1.");
            exit(1);
         }
         int scale=(int)fn;

         logSysDest->LogInfo("Iteration i=%d -> scale=%d", iCurr, scale);

         logSysDest->LogInfo("Creating random input\n");
         auto input=puzzle->CreateInput(logSysDest.get(), scale);
         auto got=puzzle->MakeEmptyOutput(input.get());

         logSysDest->LogInfo("Puzzle::Execute starts");
         double start=puzzler::now()*1e-9;
         puzzle->Execute(logUserFile.get(), input.get(), got.get());
         double finish=puzzler::now()*1e-9;
         logSysDest->LogInfo("Puzzle::Execute finishes");

         std::stringstream tmp;
         tmp<<name<<", "<<scale<<", "<<(finish-start)<<", "<<(finish-startTime);
         std::string row=tmp.str();

         logSysDest->LogInfo("Row : %s", row.c_str());

         csvFile<<row<<std::endl;
         csvFile.flush();


         iCurr++;
      }

      logSysDest->LogInfo("Time budget expired.");
      return 0;
   }catch(std::string &msg){
      std::cerr<<"Caught error string : "<<msg<<std::endl;
      return 1;
   }catch(std::exception &e){
      std::cerr<<"Caught exception : "<<e.what()<<std::endl;
      return 1;
   }catch(...){
      std::cerr<<"Caught unknown exception."<<std::endl;
      return 1;
   }
}

