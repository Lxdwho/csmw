/**
 * @brief 注册类工厂基类；
 * 分为抽象工厂基类、抽象模板工厂、子工厂
 * 用哈希表来存储所有注册过的工厂
 * @date 2026.06.13
 */

#include "ClassFactory.h"

namespace smw { // namespace smw

AbstractClassFactoryBase::AbstractClassFactoryBase(const std::string& className, const std::string& baseClassName)
{
    m_baseClassName = baseClassName;
    m_className = className;
}

const std::string& AbstractClassFactoryBase::GetBaseClassName() const
{
    return m_baseClassName;
}
const std::string& AbstractClassFactoryBase::GetClassName() const
{
    return m_className;
}

} // namespace smw
