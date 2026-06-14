/**
 * @brief: INI 配置文件解析器，用于解析配置文件
 * @date 2026.06.13
 */ 

/**
 * 配置文件格式：
 * [Feature configuration]
 * auto_enble = true
 * device_type = tty
 * sensor_state = Not_inserted   
 * net_device = eth0
 */
#include <iostream>
#include <sstream>
#include <fstream>
#include <algorithm>
#include "ini_file.h"

namespace smw {
namespace ini {

using std::stringstream;
using std::ifstream;
using std::ofstream;
using std::string;

IniFile* IniFile::Get_instance() {
	static IniFile ini;
	return &ini;
}

IniFile::IniFile() {}
IniFile::IniFile(const string & filename) { load(filename); }
IniFile::~IniFile() {}

/**
 * @brief 去除字符串前后空格 
 * @param s 待处理字符串
 */
string IniFile::trim(string s) {
    if(s.empty()) return s;
    s.erase(0, s.find_first_not_of(" \r\n"));
    s.erase(s.find_last_not_of(" \r\n") + 1);
    return s;
}

/**
 *  从文件读取所有配置，存到m_sections
 */
bool IniFile::load(const string& filename) {
    m_filename = filename;
    m_sections.clear();

    string section; 
    string line;
    ifstream fin(filename.c_str());
    if(fin.fail()) {
        printf("loading file failed: %s is not found.\n", m_filename.c_str());
        return false;
    }
    while(std::getline(fin,line)) {
        line = trim(line);
        if(line == "") continue; //空
        if(line[0] == '#' || line[0] == ';') continue; //注释
        if(line[0] == '[') { //section
            int pos = line.find_first_of(']');
            if(pos < 0) return false;
            else {
                /* 截取[ ]中间的字符为section索引 substr(pos,len)从pos开始截取长为len的*/
                section = trim(line.substr(1, pos-1));
                /* 类名字（） = 创建一个空对象 */
                m_sections[section] = Section();
            }
        }
        else { //不是section索引，现在是key对应value了,也就是Section的map
            int pos = line.find_first_of('=');
            if(pos < 0) return false;
            else {
                string key = trim(line.substr(0, pos));
                string value = trim(line.substr(pos + 1, line.size() - pos - 1));
                value = trim(value);
                m_sections[section][key] = value;
            }
        }
    }
    return true;
}

/* 把当前配置写回文件，set函数把更改写到m_sections里面，save函数保存到ini配置文件中 */
void IniFile::save(const string& filename) {
    /* 创建一个文件输出流，并 打开文件 用于写入 */
    /* 覆盖写回，会清空原来的文件内容，然后写入新内容*/
    ofstream fout(filename.c_str());
    if(fout.fail()) {
        printf("opening file failed :%s.\n", m_filename.c_str());
        return;
    }
    fout << to_str();
    fout.close();
}

/**
 * 把 整个 m_sections 转换成 ini 格式文本，用于 save() 和 show()
 */
string IniFile::to_str() {
    stringstream ss;
    for(auto it = m_sections.begin(); it != m_sections.end(); it++) {
        ss << "[" << it->first << "]" << std::endl;
        {
            for( auto iter = it->second.begin(); iter != it->second.end(); iter++) {
                ss << iter->first << " = " << (string)iter->second << std:: endl;
            }
            ss << std::endl;
        }
    }
    return ss.str();
}

/* 打印所有配置 */
void IniFile::show() {
    std::cout << to_str();
}

/* 清空所有配置 */
void IniFile::clear() {
    m_sections.clear();
}

/* 配置文件已经加载到了m_sections中，所以从m_sections中获取配置值 */
Value& IniFile::get(const string& section, const string& key) {
    return m_sections[section][key];
}

/* 用户更改/设置配置，存到m_sections */
void IniFile::set(const string& section, const string& key, const Value& value) {
    m_sections[section][key] = value;
}

/* 判断 section 是否存在 */
bool IniFile::has(const string& section) {
    return(m_sections.find(section) != m_sections.end());
}

/* 判断 key 是否存在 */
bool IniFile::has(const string& section, const string& key) {
    auto it = m_sections.find(section);
    if(it != m_sections.end()) {
        return(it->second.find(key) != it->second.end());
    }
    return false;
}

/* 删除整个section */
void IniFile::remove(const string& section) {
    m_sections.erase(section);
}

/* 删除某个key */
void IniFile::remove(const string& section, const string& key) {
    auto it = m_sections.find(section);
    if( it != m_sections.end()) {
        it->second.erase(key);
    }
}

/* 获取所有 section 名 */
std::vector<std::string> IniFile::GetSectionNames() const {
    std::vector<std::string> names;
    for (const auto& pair : m_sections) {
        names.push_back(pair.first);
    }
    return names;
}

} // namespace ini
} // namespace smw
