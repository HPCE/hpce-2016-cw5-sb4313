#ifndef puzzle_file_log_hpp
#define puzzle_file_log_hpp

#include "puzzler/core/util.hpp"

namespace puzzler
{
    /* Logs messages at one level to a file, then sends on to
        another logger.*/
  class FileLogDest
    : public ILog
  {
  private:
        std::string m_prefix;
        FILE *m_dst;
        std::shared_ptr<ILog> m_baseLog;

    std::string render(const char *str, va_list args)
    {
      std::vector<char> tmp(2000, 0);

      unsigned n=vsnprintf(&tmp[0], tmp.size(), str, args);
      if(n>tmp.size()){
        tmp.resize(n);
        vsnprintf(&tmp[0], tmp.size(), str, args);
      }

      return std::string(&tmp[0]);
    }
  public:
    FileLogDest(std::string fileName, std::string prefix, int logLevel, std::shared_ptr<ILog> baseLog)
      : ILog(logLevel)
      , m_prefix(prefix)
      , m_dst(NULL)
      , m_baseLog(baseLog)
    {
        m_dst=fopen(fileName.c_str(), "wt");
        if(!m_dst)
            throw std::runtime_error("Couldn't open log target '"+fileName+"'");
    }

    ~FileLogDest()
    {
        if(m_dst){
            fclose(m_dst);
            m_dst=0;
        }
    }

    virtual void vLog(int level, const char *str, va_list args) override
    {
      if(level<=m_logLevel){
        double t=now()*1e-9;
        std::string msg=render(str, args);
        fprintf(m_dst, "[%s], %.2f, %u, %s\n", m_prefix.c_str(), t, level, msg.c_str());
      }
      m_baseLog->vLog(level, str, args);
    }
  };

}; // puzzler

#endif
