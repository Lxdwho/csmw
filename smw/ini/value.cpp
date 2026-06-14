/**
 * @brief: 自定义的一个value类 支持 int double string bool转换
 * 实现类型强转
1、存储m_sections里的不同数据类型，都以字符串类型存储到Value的m_value里面；
2、当用户需要取出来时（不同的数据类型），实现Value类强转为不同数据类型的功能；
*/
#include <stdlib.h>
#include <sstream>
#include "value.h"

namespace smw {
namespace ini {

using std::stringstream;

Value::Value() : m_type(V_NULL)
{

}
/* 初始化列表，在对象创建时直接初始化成员变量 
*this指当前对象本身，即Value*即this->operator=(value) 代码复用*/
/* =其实重载了 bool 类型转化为Value */
Value::Value(bool value) : m_type(V_BOOL)
{
    *this = value;
}
Value::Value(int value) : m_type(V_INT)
{
    *this = value;
}
Value::Value(double value) : m_type(V_DOUBLE)
{
    *this = value;
}
/* C风格字符串，不需要进行类型转换了，因为m_value就是string类 */
Value::Value(const char* value) : m_value(value), m_type(V_STRING) { }
Value::Value(const std::string& value): m_value(value), m_type(V_STRING) { }
Value::~Value() { }

/* 获取当前Value类型 */
Value::Type Value::get_type() const
{
    return m_type;
}
/* 设置Value类型 */
void Value::set_type(Type type)
{
    m_type = type;
}
/* 判断类型，const表示判断类型不应该修改对象 */
/* 在强转前先判断，避免报错 */
bool Value::Type_isnull() const
{
    return m_type == V_NULL;
}
bool Value::Type_isint() const
{
    return m_type == V_INT;
}
bool Value::Type_isdouble() const
{
    return m_type == V_DOUBLE;
}
bool Value::Type_isstring() const
{
    return m_type == V_STRING;
}

/**
 * 定义:把其他类型赋值给Value
 * 赋值运算符重载
 * m_value是string类
 * 返回值：引用别名，减少一次对象拷贝
 */
Value& Value::operator = (bool value)
{
    m_type = V_BOOL;
    m_value = value ? "true" : "false";
    return *this;
}
Value& Value::operator = (int value)
{
    m_type = V_INT;
    /* stringstream 实现其他类型转化为字符串类型 */
    stringstream ss;
    ss << value;
    m_value = ss.str();
    return *this;
}
Value& Value::operator = (double value)
{
    m_type = V_DOUBLE;
    stringstream ss;
    ss << value;
    m_value = ss.str();
    return *this;
}
Value& Value::operator = (const char* value)
{
    m_type = V_STRING;
    m_value = value;
    return *this;
}
/* ini_file里面的string类型的value赋值给Value类*/
/* string& 地址传递，不用拷贝，节省，同时加个const保证了不修改变量*/
Value& Value::operator = (const std::string& value)
{
    m_type = V_STRING;
    m_value = value;
    return *this;

}

/* 拷贝赋值运算符重载！！ Value a,Value b  a = b*/
Value& Value::operator = (const Value& value)
{
    m_type = value.m_type;
    m_value = value.m_value;
    return *this;
}

/* 运算符重载 */
bool Value::operator == (const Value& other)
{
    if(m_type != other.m_type)
    {
        return false;
    }
    return (m_value == other.m_value);
}
bool Value::operator != (const Value& other)
{
    if(m_type != other.m_type) return true;
    return !(m_value == other.m_value);
    //return !(*this == other);
}

/**
 * 隐式地转换为 int类型 和 double 类型
 * 将Value类型，强转为其他类型
 * 重载了强转类型
 * 外部用户需要啥类型就转化成啥类型
 * 因此：
Value v = 123;
int x = v;
double d = v;
string s = v;
    */
Value::operator int()
{
    return std::stoi(m_value);
}
Value::operator double()
{
    return std::stof(m_value);
}
Value::operator bool()
{
    if(m_value == "true")
        return true;
    else if(m_value == "false")
        return false;
    return false;
}
// Value::operator std::string()
// {
//     return m_value;
// }
Value::operator std::string() const
{
    return m_value;
}

} // namespace ini
} // namespace smw
