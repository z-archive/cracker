#include <iostream>
#include <cassert>
#include "md5.h"
#include <boost/thread.hpp>
#include <boost/shared_ptr.hpp>

char c_lower = 32;
char c_upper = 127;

class Generator
{
public:
    Generator(std::size_t length
              , const std::string& begin = ""
              , const std::string& end = "") :
        m_length(length)
        , m_current(begin)
        , m_end(end)
    {
        assert(m_length > 0);
        if (m_current.empty())
        {
            m_current = std::string(m_length, c_lower);
        }
        if (m_end.empty())
        {
            m_end = std::string(m_length, c_upper);
        }
        assert(m_length == m_current.size());
        assert(m_length == m_end.size());
        m_current += "@http://facebook.com/";
        m_end += "@http://facebook.com/";
    }

    bool next()
    {
        if (m_current == m_end)
        {
            return false;
        }
        std::size_t index = m_length ;
        while (index > 0)
        {
            if (add(index - 1))
            {
                return true;
            }
            else
            {
                --index;
            }
        }
        return index > 0;
    }

    const std::string& current() const
    {
        return m_current;
    }

private:
    bool add(std::size_t index)
    {
        assert(index < m_length);
        if (m_current[index] == c_upper)
        {
            m_current[index] = c_lower;
            return false;
        }
        else
        {
            m_current[index] += 1;
            return true;
        }
    }

private:
    std::size_t m_length;
    std::string m_begin;
    std::string m_current;
    std::string m_end;
};

const std::size_t PROC_COUNT=8;

class Searcher : public Generator
{
public:
    Searcher(const MD5& target
             , std::size_t length
             , const std::string& begin = ""
             , const std::string& end = "") :
        Generator(length, begin, end)
        , m_target(target)
    {
    }

    bool search()
    {
        while (current_md5() != m_target)
        {
            if (!next())
            {
                return false;
            }
        }
        return current_md5() == m_target;
    }

private:
    MD5 current_md5() const
    {
        return MD5(current());
    }

private:
    MD5       m_target;
};

class ParallelSearcher
{
public:
    ParallelSearcher(const MD5& target, std::size_t length) :
        m_target(target), m_length(length)
    {
    }

    void search()
    {
        typedef boost::shared_ptr<boost::thread> pointer;
        std::vector<pointer> tv(PROC_COUNT);
        do
        {
            boost::thread_group tg;

            for (std::size_t i = 0; i < tv.size(); ++i)
            {
                tv[i] = pointer(new boost::thread(&ParallelSearcher::work, this, i));
            }
            for (std::size_t i = 0; i < tv.size(); ++i)
            {
                tg.add_thread(tv[i].get());
            }
            tg.join_all();
            for (std::size_t i = 0; i < tv.size(); ++i)
            {
                tg.remove_thread(tv[i].get());
            }
        }
        while (false);
    }

    void work(std::size_t index)
    {
        std::string start(m_length, c_lower);
        std::string stop(m_length, c_upper);
        start[0] = c_lower + (c_upper + 1 - c_lower) * index / PROC_COUNT;
        stop[0] =  c_lower + ((c_upper + 1 - c_lower) * (index+1) / PROC_COUNT) - 1;
        {
            std::ostringstream sstart;
            sstart << "start[" << index << "] = '" << start << "'" << std::endl;
            //std::cout << sstart.str() << std::flush;
            std::ostringstream sstop;
            sstop << "stop[" << index << "] = '" << stop << "'" << std::endl;
            //std::cout << sstop.str() << std::flush;
        }
        Searcher s(m_target, m_length, start, stop);
        do
        {
            if (s.search())
            {
                std::cout << index << " " << s.current() + '\n' << std::flush;
            }
        }
        while(s.next());
    }
private:
    const MD5 m_target;
    const std::size_t m_length;
};

int main(int, char* argv[])
{
    MD5 target(MD5::fromHex("71b8148bf74bf8e40a00bd5385a47472"));
    ParallelSearcher p(target, 6);
    p.search();
    return 0;
}
