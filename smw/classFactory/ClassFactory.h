/**
 * @brief 注册类工厂基类；
 * 分为抽象工厂基类、抽象模板工厂、子工厂
 * 用哈希表来存储所有注册过的工厂
 * @date 2026.06.13
 */

#ifndef _SMW_CLASSFACTORY_CLASSFACTORY_H_
#define _SMW_CLASSFACTORY_CLASSFACTORY_H_

#include <string>
#include <unordered_map>
#include <memory>
#include <iostream>

namespace smw {

/**
 * @brief 工厂基类：只存类名，不涉及模板，让全局表能统一存放所有工厂
 */
class AbstractClassFactoryBase {
public:
    AbstractClassFactoryBase(const std::string& className, const std::string& baseClassName);
    virtual ~AbstractClassFactoryBase() = default;

    const std::string& GetClassName() const;
    const std::string& GetBaseClassName() const;
protected:
    std::string m_className;
    std::string m_baseClassName;
};

/**
 * @brief 模板抽象工厂：定义统一接口 CreateObj  统一某一类Base的创建接口
 * @tparam Base 你要创建的对象的基类（比如 Animal）
 */
template <typename Base>
class AbstractClassFactory : public AbstractClassFactoryBase{
public:
    AbstractClassFactory(const std::string& className, const std::string& baseClassName)
        : AbstractClassFactoryBase(className, baseClassName) { }
    
    /**
     * @brief 创建对象: 所有工厂都必须实现：创建一个Base类型对象
     * @return 返回unique_ptr<Base>智能指针
     */
    virtual std::unique_ptr<Base> CreateObj() const = 0;
};

/**
 * @brief 子工厂
 * @tparam Derived 具体子类（如 Dog）
 * @tparam Base 基类（如 Animal）
 */
template <typename Derived, typename Base>
class ClassFactory : public AbstractClassFactory<Base> {
public:
    ClassFactory(const std::string& className, const std::string &baseClassName)
        :AbstractClassFactory<Base>(className, baseClassName) { }

    std::unique_ptr<Base> CreateObj() const override{
        return std::make_unique<Derived>();
    }
};

using factorMap = std::unordered_map<
                    std::string, std::unique_ptr<AbstractClassFactoryBase>>;
/**
 * @brief 获取全局唯一的工厂哈希表 类名 → 工厂对象
 * @return factorMap 注册工厂表 
 */
inline factorMap& GetClassFactoryMap() {
    static factorMap g_classFactoryMap;
    return g_classFactoryMap;
}

/**
 * @brief 将工厂注册移动到全局表
 * @param className 子类名称——字符串
 * @param factory 工厂unique指针
 */
inline void RegisterClassFactory(const std::string& className, 
        std::unique_ptr<AbstractClassFactoryBase> factory) {
    GetClassFactoryMap()[className] = std::move(factory);
}

/**
 * @brief 创建工厂，并注册到表中
 * @tparam Derived 子类工厂类别
 * @tparam Base 父类工厂类别
 * @param className 子类工厂名称
 * @param baseClassName  父类工厂名称
 */
template <typename Derived, typename Base>
void RegisterClass(const std::string& className, const std::string& baseClassName) {
    auto factory = std::make_unique<ClassFactory<Derived, Base>>(className, baseClassName);
    RegisterClassFactory(className, std::move(factory));
}

/**
 * @brief 创建对象，供用户使用
 * @tparam Base 基类类别
 * @param className 子类名称
 */
template <typename Base>
std::unique_ptr<Base> CreateObject(const std::string& className) {
    auto &factoryMap = GetClassFactoryMap();
    auto it = factoryMap.find(className);
    if(it == factoryMap.end()) {
        std::cerr << "类没有注册：" << className << std::endl;
        return nullptr;
    }
    auto* factory = dynamic_cast<const AbstractClassFactory<Base>*>(it->second.get());
    if(!factory) {
        std::cerr << "工厂类型不匹配：" << className << std::endl;
        return nullptr;
    }
    return factory->CreateObj();
}

} // namespace smw

/**
 * @brief 自动注册宏根实现
 * @param Derived 子类名称
 * @param Base 基类名称
 * @param UniqueID 工厂唯一id，防止名称冲突
 */
#define CLASS_LOADER_REGISTER_CLASS_INTERNAL(Derived, Base, UniqueID) \
namespace {                                                           \
struct ProxyType##UniqueID {                                          \
    ProxyType##UniqueID() {                                           \
        smw::RegisterClass<Derived, Base>(#Derived, #Base);           \
    }                                                                 \
};                                                                    \
static ProxyType##UniqueID g_register_class_##UniqueID;               \
}

// 辅助宏 保证 __COUNTER__ 会展开成数字
#define CLASS_LOADER_REGISTER_CLASS_INTERNAL_1(Derived, Base, UniqueID) \
    CLASS_LOADER_REGISTER_CLASS_INTERNAL(Derived, Base, UniqueID)

/**
 * @brief 工厂注册宏
 * @param Derived 子类名称
 * @param Base 基类名称
 */
#define CLASS_LOADER_REGISTER_CLASS(Derived, Base) \
    CLASS_LOADER_REGISTER_CLASS_INTERNAL_1(Derived, Base, __COUNTER__)

# endif
