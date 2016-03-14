/**
 * Copy right YMSys, 2015,2016 Zhaoming Yin
 *
 * @brief    This is the class for keeping global config 
 *
 *  MODIFIED   (MM/DD/YY)
 *  stplaydog   03/09/16 - Creation
 *
**/

#pragma once
#ifndef __H_CONFIG__
#define __H_CONFIG__

#include "utils.h"
#include <map>

using namespace std;


/**
 * @class Config
 *
 * @note  This is class for keeping global configuration 
 *
**/
class Config 
{
public:
    static Config *instance()
    {
        if(!m_instance)
        {
            m_instance = new Config;
        }
        return m_instance;
    }

    void set(const string &key, const string &val)
    {
        m_params[key] = val;
    }

    string get(const string &key)
    {
        string ret;
        ret = m_params[key];
        return ret;
    }


private:
    Config() {}
    
    static Config *     m_instance;
    
    map<string, string> m_params;
};



#endif
