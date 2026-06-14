/**
 * @brief: 自定义的一个value类 支持 int double string bool转换
 * 实现类型强转
1、存储m_sections里的不同数据类型，都以字符串类型存储到Value的m_value里面；
2、当用户需要取出来时（不同的数据类型），实现Value类强转为不同数据类型的功能；
*/
#ifndef _SMW_INI_VALUE_H_
#define _SMW_INI_VALUE_H_

#include <string>
#include <iostream>

namespace smw {
namespace ini {

class Value
{
public:
    /* 枚举记录原始类型 */
    enum Type
    {
        V_NULL = 0,
        V_BOOL,
        V_INT,
        V_DOUBLE,
        V_STRING
    };

    /**
     * 构造函数，对应实现多种参数类型的构造
     */
    Value();
    Value(bool value);
    Value(int value);
    Value(double value);
    Value(const char* value); /* C风格字符串 */
    Value(const std::string& value);
    ~Value();

    /* 获取当前Value类型 */
    Type get_type() const;
    /* 设置Value类型 */
    void set_type(Type type);
    /* 判断类型，const表示判断类型不应该修改对象 */
    /* 在强转前先判断，避免报错 */
    bool Type_isnull() const;
    bool Type_isint() const;
    bool Type_isdouble() const;
    bool Type_isstring() const;

    /**
     * 定义:把其他类型赋值给Value
     * 赋值运算符重载
     * 返回值：引用别名，减少一次对象拷贝
     */
    Value& operator = (bool value);
    Value& operator = (int value);
    Value& operator = (double value);
    Value& operator = (const char* value);
    /* ini_file里面的string类型的value赋值给Value类*/
    /* string& 地址传递，不用拷贝，节省，同时加个const保证了不修改变量*/
    Value& operator = (const std::string& value);
    /* 拷贝赋值运算符重载！！ Value a,Value b  a = b*/
    Value& operator = (const Value& value);

    /* 运算符重载 */
    bool operator == (const Value& other);
    bool operator != (const Value& other);

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
    operator int();
    operator double();
    operator bool();
    // operator std::string();
    operator std::string() const;
    
private:
    std::string m_value; //存数据
    Type m_type; //用于记录这个字符串原本是什么类型
};

} // namespace ini
} // namespace smw

#endif // _SMW_INI_VALUE_H_
