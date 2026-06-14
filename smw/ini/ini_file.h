/**
 * @brief: INI 配置文件解析器，用于解析配置文件
 * @date 2026.06.13
 */ 

#ifndef _SMW_INI_INIFILE_H_
#define _SMW_INI_INIFILE_H_

#include <string.h>
#include <string>
#include <map>
#include <vector>
#include "value.h"
#include "../common/macros.h"


#define Ini_Init(file_name) smw::ini::IniFile::Get_instance()->load(file_name)

namespace smw {
namespace ini {

using std::string;

typedef std::map<string, Value> Section;
class IniFile {
public:
    /* 从文件读取配置 */
    bool load(const string& filename);
    /* 把当前配置写回文件，set函数写到m_sections里面，save函数保存到ini配置文件中 */
    void save(const string& filename);

    /**
     * 把 m_sections 转换成 ini 格式文本，用于 save() 和 show()
     */
    string to_str();

    /* 打印所有配置 */
    void show();
    /* 清空所有配置 */
    void clear();

    /* 获取配置值 */
    Value& get(const string& section, const string& key);
    /* 设置配置，存到m_sections */
    void set(const string& section, const string& key, const Value& value);

    /* 判断 section 是否存在 */
    bool has(const string& section);
    /* 判断 key 是否存在 */
    bool has(const string& section, const string& key);

    /* 获取所有 section 名（用于遍历配置） */
    std::vector<string> GetSectionNames() const;

    /* 删除整个section */
    void remove(const string& section);
    /* 删除某个key */
    void remove(const string& section, const string& key);

    /* 返回引用Section&，可以修改m_sections */
    Section& operator [](const string& section) {
        return m_sections[section];
    }

    static IniFile* Get_instance();

private:
    IniFile();
    IniFile(const string & filename);
    ~IniFile();

    string trim(string s);

    string m_filename;
    std::map<string,Section> m_sections;
};

} // namespace ini
} // namespace smw

#endif // _SMW_INI_INIFILE_H_
