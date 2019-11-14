/*
 *  objecthandle.h
 *
 *  This file is part of GammaRay, the Qt application inspection and
 *  manipulation tool.
 *
 *  Copyright (C) 2018 Klarälvdalens Datakonsult AB, a KDAB Group company, info@kdab.com
 *  Author: Anton Kreuzkamp <anton.kreuzkamp@kdab.com>
 *
 *  Licensees holding valid commercial KDAB GammaRay licenses may use this file in
 *  accordance with GammaRay Commercial License Agreement provided with the Software.
 *
 *  Contact info@kdab.com if any conditions of this licensing are not clear to you.
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef GAMMARAY_OBJECTHANDLE_H
#define GAMMARAY_OBJECTHANDLE_H

#include <core/probe.h>
#include <common/objectid.h>

#include <QObject>
#include <QMetaObject>
#include <QMetaProperty>
#include <QDebug>
#include <QMetaType>
#include <QMutex>
#include <QSemaphore>
#include <QThread>

#include <core/metaobject.h>
#include <core/metaproperty.h>

#include <utility>
#include <future>
#include <tuple>
#include <memory>
#include <typeindex>

#include <iostream>

#include <list>

#include <private/qobject_p.h>
#include <private/qmetaobject_p.h>

template<typename T> class error;

template<int i> class err;

/**
 * May only be used in cases, where non-constexpr if would still produce valid code
 */
#define IF_CONSTEXPR if


enum ObjectWrapperFlag {
    NoFlags = 0,
    Getter = 1,
    MemberVar = 2,
    DptrGetter = 4,
    DptrMember = 8,
    CustomCommand = 16,

    QProp = 128,
    OwningPointer = 256,
    NonOwningPointer = 512,
    NonConst = 1024,
};


/**
 * Defines a getter function with the name @p FieldName, which returns the data
 * stored at index @p StorageIndex in d->dataStorage or - in the
 * non-caching case - the live value of the property.
 *
 * This is internal for use in other macros.
 */
#define DEFINE_GETTER(FieldName, StorageIndex, Flags) \
decltype(fetch_##FieldName<Flags>(static_cast<value_type*>(nullptr))) FieldName() const \
{ \
    d_ptr()->semaphore.acquire(); \
    QSemaphoreReleaser releaser { &d_ptr()->semaphore }; \
 \
    IF_CONSTEXPR (cachingDisabled<ThisClass_t>::value) { \
        return fetch_##FieldName<Flags>(object()); \
    } else { \
        return d_ptr()->cache<value_type>()->get< StorageIndex >(); \
    } \
} \

/**
 * Defines a setter function with the name @p SetterName, which sets the cached
 * value stored at index @p StorageIndex in d->dataStorage and
 * (in the multi-threaded case deferredly) updates the actual property value.
 * In the non-caching case it just sets the live value of the property directly.
 *
 * This is internal for use in other macros.
 */
#define DEFINE_SETTER(FieldName, SetterName, StorageIndex, Flags) \
void SetterName(decltype(fetch_##FieldName<Flags>(static_cast<value_type*>(nullptr))) newValue) \
{ \
    d_ptr()->semaphore.acquire(); \
    QSemaphoreReleaser releaser { &d_ptr()->semaphore }; \
    \
    IF_CONSTEXPR (cachingDisabled<ThisClass_t>::value) { \
        write_##FieldName<Flags>(object(), unwrap(newValue)); \
    } else { \
        d_ptr()->cache<value_type>()->get< StorageIndex >() = newValue; \
        /* TODO Defer calling the setter */ \
        write_##FieldName<Flags>(object(), unwrap(newValue)); \
    } \
} \


/**
 * Defines a wrapper function for direct access to the property, abstracting
 * away the different kinds of properties (getter, member variable, custom
 * command).  This differs from the DEFINE_GETTER in that the fetch function
 * never caches things. Instead it's used to update the cache.
 *
 * This is internal for use in other macros.
 */
#define DEFINE_FETCH_FUNCTION_PROP(FieldName) \
template<int Flags, typename T = pimplClass_t<ThisClass_t>, typename std::enable_if<(Flags & DptrGetter) != 0>::type* = nullptr> /*FIXME T must be the private class! */ \
static auto fetch_##FieldName(const value_type *object) \
-> decltype(wrap<Flags>(std::declval<T>().FieldName())) \
{ \
    return wrap<Flags>(static_cast<const T*>(T::get(object))->FieldName()); \
} \
template<int Flags, typename T = pimplClass_t<ThisClass_t>, typename std::enable_if<(Flags & DptrMember) != 0>::type* = nullptr> /*FIXME T must be the private class! */ \
static auto fetch_##FieldName(const value_type *object) \
-> decltype(wrap<Flags>(std::declval<T>().FieldName)) \
{ \
    return wrap<Flags>(static_cast<const T*>(T::get(object))->FieldName); \
} \
template<int Flags, typename T = value_type, typename std::enable_if<(Flags & Getter) != 0 && (Flags & NonConst) == 0>::type* = nullptr> \
static auto fetch_##FieldName(const T *object) \
-> decltype(wrap<Flags>(std::declval<T>().FieldName())) \
{ \
    return wrap<Flags>(object->FieldName()); \
} \
template<int Flags, typename T = value_type, typename std::enable_if<(Flags & Getter) != 0 && (Flags & NonConst) != 0>::type* = nullptr> \
static auto fetch_##FieldName(T *object) \
-> decltype(wrap<Flags>(std::declval<T>().FieldName())) \
{ \
    return wrap<Flags>(object->FieldName()); \
} \
template<int Flags, typename T = value_type, typename std::enable_if<(Flags & MemberVar) != 0>::type* = nullptr> \
static auto fetch_##FieldName(const T *object) \
-> decltype(wrap<Flags>(std::declval<T>().FieldName)) \
{ \
    return wrap<Flags>(object->FieldName); \
} \

/**
 * Like DEFINE_FETCH_FUNCTION_PROP but with custom expression.
 *
 * This is internal for use in other macros.
 */
#define DEFINE_FETCH_FUNCTION_CUSTOM_EXPRESSION(FieldName, Expr) \
template<int Flags, typename T = value_type, typename std::enable_if<(Flags & NonConst) == 0>::type* = nullptr> \
static auto fetch_##FieldName(const T *object) \
-> decltype(wrap<Flags>(Expr)) \
{ \
    return wrap<Flags>(Expr); \
} \
template<int Flags, typename T = value_type, typename std::enable_if<(Flags & NonConst) != 0>::type* = nullptr> \
static auto fetch_##FieldName(T *object) \
-> decltype(wrap<Flags>(Expr)) \
{ \
    return wrap<Flags>(Expr); \
} \


/**
 * Defines a wrapper function for direct write access to the property,
 * abstracting away the different methods of writing properties (setter, member
 * variable, custom command).  This differs from the DEFINE_SETTER in that the
 * write function never caches things. Instead it's used to update the cache.
 *
 * This is internal for use in other macros.
 */
#define DEFINE_WRITE_FUNCTION_PROP(FieldName, SetterName) \
template<int Flags, typename T = pimplClass_t<ThisClass_t>, typename std::enable_if<(Flags & DptrGetter) != 0>::type* = nullptr> /*FIXME T must be the private class! */ \
static void write_##FieldName(value_type *object, decltype(std::declval<T>().FieldName()) newVal) \
{ \
    static_cast<T*>(T::get(object))->SetterName(newVal); \
} \
template<int Flags, typename T = pimplClass_t<ThisClass_t>, typename std::enable_if<(Flags & DptrMember) != 0>::type* = nullptr> /*FIXME T must be the private class! */ \
static void write_##FieldName(value_type *object, decltype(std::declval<T>().FieldName) newVal) \
{ \
    static_cast<T*>(T::get(object))->FieldName = newVal; \
} \
template<int Flags, typename T = value_type, typename std::enable_if<(Flags & Getter) != 0>::type* = nullptr> \
static void write_##FieldName(T *object, decltype(std::declval<T>().FieldName()) newVal) \
{ \
    object->SetterName(newVal); \
} \
template<int Flags, typename T = value_type, typename std::enable_if<(Flags & MemberVar) != 0>::type* = nullptr> \
static void write_##FieldName(T *object, decltype(std::declval<T>().FieldName) newVal) \
{ \
    object->FieldName = newVal; \
} \



template<typename T, typename ...Args>
struct tuple_append {};
template<typename T, typename ...TupleArgs_t>
struct tuple_append<std::tuple<TupleArgs_t...>, T> {
    using type = std::tuple<TupleArgs_t..., T>;
};
template<typename Tuple_t, typename T>
using tuple_append_t = typename tuple_append<Tuple_t, T>::type;


/**
 * Defines a method, which holds state using overloading and inheritance tricks
 * as described at https://woboq.com/blog/verdigris-implementation-tricks.html
 *
 * Incrementing the counter is done by using a combination of DATA_APPEND and
 * DEFINE_COUNTER. Use DEFINE_COUNTER to define a constexpr variable holding the
 * current count + 1. Use DATA_APPEND to make it the new current count.
 *
 * Use StateExpression to return arbitary content, possibly using recursion. Eg.
 * you can do
 * `std::tuple_cat(MethodName(self, __counter.prev()), std::make_tuple(...)))`
 * to recursively compose a compile-time list.
 *
 * This is meant for internal use in other macros only.
 */
#define DATA_APPEND(Counter, AppendedType, StateExpr) \
static auto __data(ObjectWrapperPrivate *d, __number<Counter> __counter) \
-> tuple_append_t<decltype(__data(d, __counter.prev())), AppendedType> \
{ \
    return std::tuple_cat(__data(d, __counter.prev()), std::make_tuple(StateExpr)); \
}

/**
 * Defines an overload to __metadata, which adds \p FieldName to the metaobject
 * and recursively calls the previously defined overload of __metadata.
 * (see https://woboq.com/blog/verdigris-implementation-tricks.html).
 *
 * This is meant for internal use in other macros only.
 */
#define ADD_TO_METAOBJECT(FieldName, FieldType, Flags) \
static void __metadata(__number<W_COUNTER_##FieldName>, MetaObject *mo) \
{ \
    mo->addProperty(GammaRay::MetaPropertyFactory::makeProperty(#FieldName, &ThisClass_t::FieldName)); \
    __metadata(__number< W_COUNTER_##FieldName - 1 >{}, mo); \
}

/**
 * Defines a constexpr variable that fetches the current value of the count
 * used by the constexpr-state code defined by STATE_APPEND and stores it
 * incremented by one.
 *
 * @p CounterName is the name of the variable to be defined
 * @p CounterMethod is the name of the method, which holds the state. This
 *                  method must return a tuple, whose size equals the
 *                  current count.
 *
 * Incrementing the counter is done by using a combination of STATE_APPEND and
 * DEFINE_COUNTER. Use DEFINE_COUNTER to define a constexpr variable holding the
 * current count + 1. Use STATE_APPEND to make it the new current count.
 *
 * This is meant for internal use in other macros only.
 */
#define DEFINE_COUNTER(CounterName, CounterMethod) \
static constexpr int CounterName = \
std::tuple_size<decltype(CounterMethod(static_cast<ObjectWrapperPrivate *>(nullptr), __number<255>{}))>::value + 1; \


/**
 * Defines an overload to __connectToUpdates, which connects an update property
 * slot to the notify signal of the property \p FieldName. It then recursively
 * calls the previously defined overload of __connectToUpdates
 * (see https://woboq.com/blog/verdigris-implementation-tricks.html).
 *
 * This is meant for internal use in other macros only.
 */
#define CONNECT_TO_UPDATES(FieldName, Flags) \
static void __connectToUpdates(ObjectWrapperPrivate *d, __number<W_COUNTER_##FieldName>) \
{ \
    d->connectToUpdates< value_type, W_COUNTER_##FieldName - 1, Flags >(&ThisClass_t::fetch_##FieldName<Flags>, #FieldName); \
    __connectToUpdates(d, __number< W_COUNTER_##FieldName - 1 >{}); \
} \


/**
 * Adds a property to the object wrapper. The data will be accessible
 * through a getter in the wrapper, named as \p FieldName.
 *
 * The property can be customized by a couple of \p Flags:
 *  Getter: If this flag is set, data will be fetched using obj->FieldName()
 *  MemberVar: Data will be fetched by accessing the member field obj->FieldName directly
 *  DptrGetter: Data will be fetched by accessing ClassPrivate::get(obj)->FieldName()
 *  DptrMember: Data will be fetched by accessing ClassPrivate::get(obj)->FieldName
 *  CustomCommand: Incompatible with this macro. Use CUSTOM_PROP instead.
 *
 *  NonConst: Indicates that the getter is non-const, thus making the wrapped
 *            getter non-const, too.
 *  QProp: Indicates that there exists a Qt property with this name. Setting
 *         this flag will enable reading, writing as well as automatic updating.
 *  OwningPointer: Indicates that the object owns the object which this property
 *                 points to. Setting this correctly is crucial for memory
 *                 management of the object wrapper.
 *  NonOwningPointer Indicates that this object does not own the object which
 *                   this property points to. Setting this correctly is crucial
 *                   for memory management of the object wrapper.
 *
 * It is necessary to set one of Getter/MemberVar/DptrGetter/DptrMember.
 * Further, for properties that are pointers to other wrappable
 * objects, it's necessary to set either OwningPointer or NonOwningPointer.
 *
 * Example: If you used obj->x() before to access some data, you can make that
 * available to the wrapper, by writing `PROP(x, Getter)`. Later, use wrapper.x()
 * to access it.
 */
#define RO_PROP(FieldName, Flags) \
DEFINE_COUNTER(W_COUNTER_##FieldName, __data) \
DEFINE_FETCH_FUNCTION_PROP(FieldName) \
DATA_APPEND(W_COUNTER_##FieldName, decltype(fetch_##FieldName<Flags>(static_cast<value_type*>(nullptr))), fetch_##FieldName<Flags>(d->object<value_type>())) \
DEFINE_GETTER(FieldName, W_COUNTER_##FieldName - 1, Flags) \
ADD_TO_METAOBJECT(FieldName, decltype(fetch_##FieldName<Flags>(static_cast<value_type*>(nullptr))), Flags) \
CONNECT_TO_UPDATES(FieldName, Flags) \


/**
 * Adds a property to the object wrapper. The data will be accessible
 * through a getter in the wrapper, named as \p FieldName. Data will be writable
 * through a setter in the wrapper, named as \p SetterName.
 *
 * The property can be customized by a couple of \p Flags:
 *  Getter: If this flag is set, data will be fetched using obj->FieldName()
 *          and written using obj->SetterName(newVal).
 *  MemberVar: Data will be fetched/written by accessing the member field
 *             obj->FieldName directly.
 *  DptrGetter: Data will be fetched by accessing ClassPrivate::get(obj)->FieldName()
 *              and written using ClassPrivate::get(obj)->SetterName(newVal).
 *  DptrMember: Data will be fetched/written by accessing
 *              ClassPrivate::get(obj)->FieldName.
 *  CustomCommand: Incompatible with this macro. Use CUSTOM_PROP instead.
 *
 *  NonConst: Indicates that the getter is non-const, thus making the wrapped
 *            getter non-const, too.
 *  QProp: Indicates that there exists a Qt property with this name. Setting
 *         this flag will enable reading, writing as well as automatic updating.
 *  OwningPointer: Indicates that the object owns the object which this property
 *                 points to. Setting this correctly is crucial for memory
 *                 management of the object wrapper.
 *  NonOwningPointer Indicates that this object does not own the object which
 *                   this property points to. Setting this correctly is crucial
 *                   for memory management of the object wrapper.
 *
 * It is necessary to set one of Getter/MemberVar/DptrGetter/DptrMember.
 * Further, for properties that are pointers to other wrappable
 * objects, it's necessary to set either OwningPointer or NonOwningPointer.
 *
 * Example: If you used obj->x() before to access some data, you can make that
 * available to the wrapper, by writing `PROP(x, Getter)`. Later, use wrapper.x()
 * to access it.
 */
#define RW_PROP(FieldName, SetterName, Flags) \
DEFINE_COUNTER(W_COUNTER_##FieldName, __data) \
DEFINE_FETCH_FUNCTION_PROP(FieldName) \
DEFINE_WRITE_FUNCTION_PROP(FieldName, SetterName) \
DATA_APPEND(W_COUNTER_##FieldName, decltype(fetch_##FieldName<Flags>(static_cast<value_type*>(nullptr))), fetch_##FieldName<Flags>(d->object<value_type>())) \
DEFINE_GETTER(FieldName, W_COUNTER_##FieldName - 1, Flags) \
DEFINE_SETTER(FieldName, SetterName, W_COUNTER_##FieldName - 1, Flags) \
ADD_TO_METAOBJECT(FieldName, decltype(fetch_##FieldName<Flags>(static_cast<value_type*>(nullptr))), Flags) \
CONNECT_TO_UPDATES(FieldName, Flags) \

/**
 * Adds a property to the object wrapper. The data will be accessible
 * through a getter in the wrapper, named as \p FieldName. The value of the
 * property will be given by evaluating the expression \p Expression in a
 * context where `object` is a valid C-pointer pointing to the wrapped object.
 *
 * The property can be customized by a couple of \p Flags:
 *  Getter, MemberVar, DptrGetter, DptrMember: Incompatible
 *      with this macro. Use PROP instead.
 *  CustomCommand: Optional when using this macro. Indicates that data is to be
 *                 fetched using the custom command \p Expression.
 *
 *  NonConst: Indicates that the expression is non-const, thus making the wrapped
 *            getter non-const, too.
 *  QProp: Indicates that there exists a Qt property with this name. Setting
 *         this flag will enable reading, writing as well as automatic updating.
 *  OwningPointer: Indicates that the object owns the object which this property
 *                 points to. Setting this correctly is crucial for memory
 *                 management of the object wrapper.
 *  NonOwningPointer Indicates that this object does not own the object which
 *                   this property points to. Setting this correctly is crucial
 *                   for memory management of the object wrapper.
 *
 * For properties that are pointers to other wrappable objects, it's necessary
 * to set either OwningPointer or NonOwningPointer.
 *
 * Example: Let Utils::getQmlId(QQuickItem*) be defined. To add a property id to
 * the wrapper of QQuickItem, use `CUSTOM_PROP(id, Utils::getQmlId(object), CustomCommand)`.
 * Later, use wrapper.id() to access it.
 */
#define CUSTOM_PROP(FieldName, Expression, Flags) \
DEFINE_COUNTER(W_COUNTER_##FieldName, __data) \
DEFINE_FETCH_FUNCTION_CUSTOM_EXPRESSION(FieldName, Expression) \
DATA_APPEND(W_COUNTER_##FieldName, \
decltype(fetch_##FieldName<Flags | CustomCommand>(static_cast<value_type*>(nullptr))), fetch_##FieldName<Flags | CustomCommand>(d->object<value_type>())) \
DEFINE_GETTER(FieldName, W_COUNTER_##FieldName - 1, Flags | CustomCommand) \
ADD_TO_METAOBJECT(FieldName, decltype(fetch_##FieldName<Flags | CustomCommand>(static_cast<value_type*>(nullptr))), Flags | CustomCommand) \


#define DIRECT_ACCESS_METHOD(MethodName) \
template<typename ...Args> auto MethodName(Args &&...args) -> decltype(object()->MethodName(unwrap(args)...)) \
{ \
    return object()->MethodName(unwrap(args)...); \
} \
template<typename ...Args> auto MethodName(Args &&...args) const -> decltype(object()->MethodName(unwrap(args)...)) \
{ \
    return object()->MethodName(unwrap(args)...); \
} \


#define BLOCKING_ASYNC_METHOD(MethodName) \
template<typename ...Args> auto MethodName(Args &&...args) -> decltype(object()->MethodName(args...)) \
{ \
    return call(object(), &value_type::MethodName, args...).get(); \
} \

#define ASYNC_VOID_METHOD(MethodName) \
template<typename ...Args> void MethodName(Args &&...args) \
{ \
    call(object(), &value_type::MethodName, args...); \
} \

/**
 * Put this macro in the va_args area of DECLARE_OBJECT_WRAPPER to disable
 * caching for this class. Disabling caching means that accessing the wrapped
 * getters will always return the live value by accessing the underlying
 * getter/member directly.
 *
 * Disabling caching is mainly meant as a porting aid.
 */
#define DISABLE_CACHING using disableCaching_t = void;


/**
 * Put this macro in the va_args area of DECLARE_OBJECT_WRAPPER to
 * set the name of the private (pimpl-) class of the type, you're
 * wrapping. This is necessary for properties of type DptrGetter and
 * DptrMember.
 */
#define PRIVATE_CLASS(PrivateClassName) using pimpl_t = PrivateClassName;


#define OBJECT_WRAPPER_COMMON(Class, ...) \
public: \
    using value_type = Class; \
    using ThisClass_t = ObjectWrapper<Class>; \
 \
    __VA_ARGS__; \
 \
    static MetaObject *staticMetaObject() { \
        static auto mo = PropertyCache_t::createStaticMetaObject(QStringLiteral(#Class)); \
        return mo.get(); \
    } \
 \
    explicit ObjectWrapper<Class>() = default; \
 \
private: \
    friend class ObjectWrapperTest; \
    friend class ObjectHandle<Class>; \



/**
 * Defines a specialization of the dummy ObjectWrapper class template for
 * \p Class. This is the main macro for enabling wrapping capabilities for a
 * given class.
 *
 * This macro has two arguments. The first one is the name of the class to be
 * wrapped. The second argument is a free-form area that can be used to put
 * arbitrary content into the wrapper class. Its mostly meant, though, to put
 * PROP and CUSTOM_PROP macros in there, which define properties, the wrapper
 * will have. Also put DISABLE_CACHING here, if desired.
 */
#define DECLARE_OBJECT_WRAPPER(Class, ...) \
template<> \
class GammaRay::ObjectWrapper<Class> \
{ \
protected: \
    static std::tuple<> __data(ObjectWrapperPrivate *, __number<0>) { return {}; } \
    static void __metadata(__number<0>, MetaObject *) {} \
    static void __connectToUpdates(ObjectWrapperPrivate *, __number<0>) {} \
    \
    Class *object() const \
    { \
        return d->object<Class>(); \
    } \
 \
public: \
    using PropertyCache_t = PropertyCache<Class>; \
 \
    explicit ObjectWrapper<Class>(std::shared_ptr<ObjectWrapperPrivate> controlBlock) \
    : d(std::move(controlBlock)) \
    {} \
 \
    OBJECT_WRAPPER_COMMON(Class, __VA_ARGS__) \
 \
 ObjectWrapperPrivate *d_ptr() const { return d.get(); } \
 \
protected: \
    std::shared_ptr<ObjectWrapperPrivate> d; \
}; \
Q_DECLARE_METATYPE(GammaRay::ObjectWrapper<Class>) \
Q_DECLARE_METATYPE(GammaRay::ObjectHandle<Class>) \
Q_DECLARE_METATYPE(GammaRay::ObjectView<Class>)

/**
 * Defines a specialization of the dummy ObjectWrapper class template for
 * \p Class. This is the main macro for enabling wrapping capabilities for a
 * given class.
 *
 * This macro has three arguments. The first one is the name of the class to be
 * wrapped. The second argument is a (the) base class of \p Class, which already
 * has a wrapper class defined. The third argument is a free-form area that can
 * be used to put arbitrary content into the wrapper class. Its mostly meant,
 * though, to put PROP and CUSTOM_PROP macros in there, which define properties,
 * the wrapper will have. Also put DISABLE_CACHING here, if desired.
 */
#define DECLARE_OBJECT_WRAPPER_WB(Class, BaseClass, ...) \
template<> \
class GammaRay::ObjectWrapper<Class> : public GammaRay::ObjectWrapper<BaseClass> \
{ \
protected: \
    /* We hide the base classes __data and __metadata functions on purpose and start counting at 0 again. */ \
    static std::tuple<> __data(ObjectWrapperPrivate *, __number<0>) { return {}; } \
    static void __metadata(__number<0>, MetaObject *) {} \
    static void __connectToUpdates(ObjectWrapperPrivate * d, __number<0>) { ObjectWrapper<BaseClass>::__connectToUpdates(d, __number<255>{}); } \
    \
public: \
    using PropertyCache_t = PropertyCache<Class, BaseClass>; \
    \
    explicit ObjectWrapper<Class>(std::shared_ptr<ObjectWrapperPrivate> controlBlock) \
        : ObjectWrapper<BaseClass>(std::move(controlBlock)) \
    {} \
    \
    Class *object() const \
    { \
        return d->object<Class>(); \
    } \
 \
    OBJECT_WRAPPER_COMMON(Class, __VA_ARGS__) \
 \
private: \
    ObjectWrapperPrivate *d_ptr() const { return d.get(); } \
}; \
Q_DECLARE_METATYPE(GammaRay::ObjectWrapper<Class>) \
Q_DECLARE_METATYPE(GammaRay::ObjectHandle<Class>) \
Q_DECLARE_METATYPE(GammaRay::ObjectView<Class>)

/**
 * Defines a specialization of the dummy ObjectWrapper class template for
 * \p Class. This is the main macro for enabling wrapping capabilities for a
 * given class.
 *
 * This macro has three arguments. The first one is the name of the class to be
 * wrapped. The second argument is a (the) base class of \p Class, which already
 * has a wrapper class defined. The third argument is a free-form area that can
 * be used to put arbitrary content into the wrapper class. Its mostly meant,
 * though, to put PROP and CUSTOM_PROP macros in there, which define properties,
 * the wrapper will have. Also put DISABLE_CACHING here, if desired.
 */
#define DECLARE_OBJECT_WRAPPER_WB2(Class, BaseClass1, BaseClass2, ...) \
template<> \
class GammaRay::ObjectWrapper<Class> : public GammaRay::ObjectWrapper<BaseClass1>, public GammaRay::ObjectWrapper<BaseClass2> \
{ \
    protected: \
        /* We hide the base classes __data and __metadata functions on purpose and start counting at 0 again. */ \
        static std::tuple<> __data(ObjectWrapperPrivate *, __number<0>) { return {}; } \
        static void __metadata(__number<0>, MetaObject *) {} \
        static void __connectToUpdates(ObjectWrapperPrivate * d, __number<0>) { \
            ObjectWrapper<BaseClass1>::__connectToUpdates(d, __number<255>{}); \
            ObjectWrapper<BaseClass2>::__connectToUpdates(d, __number<255>{}); \
        } \
        \
        public: \
            using PropertyCache_t = PropertyCache<Class, BaseClass1, BaseClass2>; \
            \
            explicit ObjectWrapper<Class>(std::shared_ptr<ObjectWrapperPrivate> controlBlock) \
            : ObjectWrapper<BaseClass1>(controlBlock) \
            , ObjectWrapper<BaseClass2>(std::move(controlBlock)) \
            {} \
            \
            Class *object() const \
            { \
                return d_ptr()->object<Class>(); \
            } \
            \
            OBJECT_WRAPPER_COMMON(Class, __VA_ARGS__) \
 \
private: \
    ObjectWrapperPrivate *d_ptr() const { return ObjectWrapper<BaseClass1>::d.get(); } \
}; \
Q_DECLARE_METATYPE(GammaRay::ObjectWrapper<Class>) \
Q_DECLARE_METATYPE(GammaRay::ObjectHandle<Class>) \
Q_DECLARE_METATYPE(GammaRay::ObjectView<Class>)



namespace GammaRay {

template< class... > using void_t = void; // C++17: Use std::void_t.
template< class... > struct TemplateParamList {};

template<typename T, typename... Args>
std::unique_ptr<T> make_unique(Args&&... args) // C++14: Use std::make_unique
{
    return std::unique_ptr<T>(new T(std::forward<Args>(args)...));
}

template<int N> struct __number : public __number<N - 1> {
    static constexpr int value = N;
    static constexpr __number<N-1> prev() { return {}; }
};
// Specialize for 0 to break the recursion.
template<> struct __number<0> { static constexpr int value = 0; };

template<typename T1, typename T2> using second_t = T2;


template<typename T, typename Enable = void>
struct cachingDisabled : public std::false_type {};
template<typename T>
struct cachingDisabled<T, typename T::disableCaching_t> : public std::true_type {};

template<typename Class, typename ...BaseClasses> class ObjectWrapper {};

template<typename Wrapper, typename Enable = void>
struct isSpecialized : public std::false_type {};
template<typename Wrapper>
struct isSpecialized<Wrapper, void_t<typename Wrapper::ThisClass_t>>
 : public std::true_type {};

template<typename T>
using propertyCache_t = typename ObjectWrapper<T>::PropertyCache_t;

template<typename T, typename Enable = void>
struct pimplClass { using type = void; };
template<typename T>
struct pimplClass<T, void_t<typename T::pimpl_t>> { using type = typename T::pimpl_t; };
template<typename T> using pimplClass_t = typename pimplClass<T>::type;

struct PropertyCacheBase
{
    virtual ~PropertyCacheBase() = default;

    virtual PropertyCacheBase *cache(std::type_index type) = 0;

    /**
     * Just returns the object pointer as this propertycache sees it
     * (might NOT be a valid pointer to the most-derived object)
     */
    virtual void *object() const = 0;
};

class ObjectWrapperPrivate;

template<typename Class, typename ...BaseClasses>
struct PropertyCache final : PropertyCacheBase
{
    using ObjectWrapper_t = ObjectWrapper<Class>;
    using Data_t = decltype( ObjectWrapper_t::__data(static_cast<ObjectWrapperPrivate*>(nullptr), __number<255>()) );
    using value_type = Class;

    Data_t dataStorage;
    std::tuple<std::unique_ptr<propertyCache_t<BaseClasses>>...> m_baseCaches;
    Class *m_object;

    ~PropertyCache() override = default;

    explicit PropertyCache(Class *object)
        : m_baseCaches(make_unique<propertyCache_t<BaseClasses>>(object)...)
        , m_object(object)
    {
    }

    PropertyCacheBase *cache(std::type_index type) override
    {
        if (std::type_index { typeid(decltype(*this)) } == type) {
            return this;
        }

        return getCacheFromBases(TemplateParamList<BaseClasses...>{}, type); // C++17: Use fold expression
    }

    template<size_t I>
    decltype(std::get<I>(dataStorage)) get()
    {
        return std::get<I>(dataStorage);
    }

//     explicit PropertyCache(Class *object, Data_t &&dataStorage);

    void update(ObjectWrapperPrivate *d)
    {
        dataStorage = ObjectWrapper<Class>::__data(d, __number<255>());

        updateBases(d, TemplateParamList<BaseClasses...>{}); // C++17: Use fold expression
    }

    static std::unique_ptr<MetaObject> createStaticMetaObject(const QString &className) {
        auto mo = new MetaObjectImpl<Class>;
        mo->setClassName(className);
        ObjectWrapper_t::__metadata(__number<255>(), mo);
        appendBaseclassMetaObjects(mo, TemplateParamList<BaseClasses...>{}); // C++17: Use fold expression
        return std::unique_ptr<MetaObject>{mo};
    }


    void *object() const override
    {
        return m_object;
    }

private:
    template<typename Head, typename ...Rest>
    void updateBases(ObjectWrapperPrivate *d, TemplateParamList<Head, Rest...>)
    {
        constexpr size_t i = std::tuple_size<decltype(m_baseCaches)>::value - sizeof...(Rest) - 1;
        std::get<i>(m_baseCaches)->update(d);

        updateBases(d, TemplateParamList<Rest...>{});
    }
    void updateBases(ObjectWrapperPrivate *, TemplateParamList<>)
    {}

    template<typename Head, typename ...Rest>
    static void appendBaseclassMetaObjects(MetaObjectImpl<Class> *mo, TemplateParamList<Head, Rest...>)
    {
        mo->addBaseClass(ObjectWrapper<Head>::staticMetaObject());

        appendBaseclassMetaObjects(mo, TemplateParamList<Rest...>{});
    }
    static void appendBaseclassMetaObjects(MetaObjectImpl<Class> *, TemplateParamList<>)
    {}


    PropertyCacheBase *getCacheFromBases(TemplateParamList<>, std::type_index) const
    {
        return nullptr;
    }
    template<typename Head, typename ...Rest>
    PropertyCacheBase *getCacheFromBases(TemplateParamList<Head, Rest...>, std::type_index type) const
    {
        constexpr size_t i = std::tuple_size<decltype(m_baseCaches)>::value - sizeof...(Rest) - 1;
        auto ret = std::get<i>(m_baseCaches)->cache(type);

        if (!ret) {
            ret = getCacheFromBases(TemplateParamList<Rest...>{}, type);
        }

        return ret;
    }
};


class ObjectWrapperPrivate : public std::enable_shared_from_this<ObjectWrapperPrivate>
{
public:
    template<typename T>
    propertyCache_t<T> *cache() const
    {
        auto ret = dynamic_cast<propertyCache_t<T>*>(m_cache->cache(typeid(propertyCache_t<T>)));
        Q_ASSERT(ret);
        return ret;
    }

    template<typename T>
    T *object() const
    {
        return cache<T>()->m_object;
    }

    explicit ObjectWrapperPrivate(std::unique_ptr<PropertyCacheBase> cache)
     : m_cache(std::move(cache))
    {}

    inline ~ObjectWrapperPrivate();

    template<typename Class>
    static std::shared_ptr<ObjectWrapperPrivate> create(Class *object);

    template<typename Class, int storageIndex, int Flags, typename CommandFunc_t, typename std::enable_if<!(Flags & QProp)>::type* = nullptr>
    void connectToUpdates(CommandFunc_t, const char*) {}

    template<typename Class, int storageIndex, int Flags, typename CommandFunc_t, typename std::enable_if<(Flags & QProp) != 0>::type* = nullptr>
    void connectToUpdates(CommandFunc_t command, const char* propertyName);

    template<typename Class, int storageIndex, typename CommandFunc_t, typename SignalFunc_t>
    void connectToUpdates(CommandFunc_t command, SignalFunc_t signal);

    std::vector<QMetaObject::Connection> connections;
    QSemaphore semaphore { 1 };
private:
    std::unique_ptr<PropertyCacheBase> m_cache;
};

template<typename T>
class ObjectHandle
{
public:
    using value_type = T;

    static_assert(isSpecialized<ObjectWrapper<T>>::value, "Can't create ObjectHandle: ObjectWrapper is not specialized on this type. Use DECLARE_OBJECT_WRAPPER to define a sepecialization.");

    explicit ObjectHandle(std::shared_ptr<ObjectWrapperPrivate> controlBlock);
    explicit ObjectHandle(ObjectWrapper<T> wrapper) : m_d(std::move(wrapper)) {}
    explicit ObjectHandle() = default;

    explicit operator bool() const;
    explicit operator T*() const;

    template<typename U>
    operator ObjectHandle<U>()
    {
        static_assert(std::is_base_of<U, T>::value, "Cannot cast ObjectHandle to type U: U is not a baseclass of T.");
        static_assert(isSpecialized<ObjectWrapper<T>>::value, "Cannot cast ObjectHandle to baseclass U: ObjectWrapper is not specialized on type U. Use DECLARE_OBJECT_WRAPPER to define a sepecialization.");

        return ObjectHandle <U> { static_cast<ObjectWrapper<U>>(m_d) };
    }

    inline const ObjectWrapper<T> *operator->() const;
    inline ObjectWrapper<T> *operator->();
    inline const ObjectWrapper<T> &operator*() const;
    inline ObjectWrapper<T> &operator*();

    inline T *object() const;
    inline T *data() const;
    inline ObjectId objectId() const;

    template<typename Func, typename ...Args>
    auto call(Func &&f, Args &&...args) -> std::future<decltype(std::declval<T*>()->*f(args...))>;


    static MetaObject *staticMetaObject();

    void refresh();

private:
    ObjectWrapper<T> m_d;
};

template<typename T>
class ObjectView
{
public:
    explicit ObjectView() = default;
    explicit ObjectView(std::weak_ptr<ObjectWrapperPrivate> controlBlock);
    explicit operator bool() const;

    static ObjectView nullhandle();

    inline ObjectHandle<T> lock() const;

    // TODO: Do we actually want implicit locking for ObjectView? (Yes, we do!)
    inline const ObjectHandle<T> operator->() const;
    inline const ObjectWrapper<T> &operator*() const;
    inline ObjectWrapper<T> &operator*();
    inline T *object() const;
    inline ObjectId objectId() const;

private:
    std::weak_ptr<ObjectWrapperPrivate> d;
};


class ObjectShadowDataRepository
{
public:
    static inline ObjectShadowDataRepository *instance();

    template<typename Class>
    static ObjectHandle<Class> handleForObject(Class *obj);

    template<typename Class>
    static ObjectView<Class> viewForObject(Class *obj);

private:
    explicit ObjectShadowDataRepository() = default;
    friend class Probe;

    QHash<void*, std::weak_ptr<ObjectWrapperPrivate>> m_objectToWrapperPrivateMap;

    friend class ObjectWrapperPrivate;
    friend class ObjectWrapperTest;
};




// === PropertyCache ===

template<typename T>
auto checkCorrectThread(T*) -> typename std::enable_if<!std::is_base_of<QObject, T>::value, bool>::type
{
    return true;
}
template<typename T>
auto checkCorrectThread(T *obj) -> typename std::enable_if<std::is_base_of<QObject, T>::value, bool>::type
{
    return obj->thread() == QThread::currentThread();
}
template<typename T>
auto checkValidObject(T *obj) -> typename std::enable_if<!std::is_base_of<QObject, T>::value, bool>::type
{
    return obj != nullptr;
}
template<typename T>
auto checkValidObject(T *obj) -> typename std::enable_if<std::is_base_of<QObject, T>::value, bool>::type
{
    return Probe::instance()->isValidObject(obj);
}


template<typename Class>
std::shared_ptr<ObjectWrapperPrivate> ObjectWrapperPrivate::create(Class *object)
{
    if (!checkValidObject(object)) {
        return {};
    }
    Q_ASSERT_X(checkCorrectThread(object), "ObjectHandle", "ObjectHandles can only be created from the thread which the wrapped QObject belongs to.");

    // Here, nobody else can have a reference to the cache objects yet, so we don't need to
    // guard the access with a semaphore. Also we're in object's thread, so we don't need to guard
    // against asynchronous deletions of object.

    auto cache = make_unique<propertyCache_t<Class>>(object); // recursively creates all subclasses' caches
    auto d = std::make_shared<ObjectWrapperPrivate>(std::move(cache));
    ObjectWrapper<Class>::__connectToUpdates(d.get(), __number<255>{});

    return d;
}

template<typename Class, int storageIndex, int Flags, typename CommandFunc_t, typename std::enable_if<(Flags & QProp) != 0>::type*>
void ObjectWrapperPrivate::connectToUpdates(CommandFunc_t fetchFunction, const char* propertyName)
{
    static_assert(std::is_base_of<QObject, Class>::value, "members with notify signals can only be defined for QObject-derived types.");
    auto object = static_cast<QObject*>(cache<Class>()->m_object);
    auto mo = object->metaObject();
    auto prop = mo->property(mo->indexOfProperty(propertyName));

    if (!prop.hasNotifySignal()) {
        return;
    }

    auto weakSelf = std::weak_ptr<ObjectWrapperPrivate> { shared_from_this() }; // C++17: Use weak_from_this()
    auto f = [weakSelf, fetchFunction]() {
        std::cout << "Updating cache."<< storageIndex <<"\n";
        QMutexLocker locker { Probe::objectLock() };
        auto d = weakSelf.lock();
        d->semaphore.acquire();
        QSemaphoreReleaser releaser { d->semaphore };
        d->cache<Class>()->template get<storageIndex>() = fetchFunction(d->object<Class>());
    };

    auto connection = QObjectPrivate::connect(object,
                                                prop.notifySignal().methodIndex(),
                                                new QtPrivate::QFunctorSlotObjectWithNoArgs<decltype(f), void>(std::move(f)),
                                                Qt::DirectConnection
    );

    connections.push_back(connection);
}


template<typename Class, int storageIndex, typename CommandFunc_t, typename SignalFunc_t>
void ObjectWrapperPrivate::connectToUpdates(CommandFunc_t command, SignalFunc_t signal)
{
    static_assert(std::is_base_of<QObject, Class>::value, "members with notify signals can only be defined for QObject-derived types.");
    auto object = static_cast<QObject*>(cache<Class>()->m_object);
    auto weakSelf = std::weak_ptr<ObjectWrapperPrivate> { shared_from_this() };
    auto f = [weakSelf, command]() {
        std::cout << "Updating cache."<< storageIndex <<"\n";
        QMutexLocker locker { Probe::objectLock() };
        auto d = weakSelf.lock();
        d->semaphore.acquire();
        QSemaphoreReleaser releaser { d->semaphore };
        d->cache<Class>()->template get<storageIndex>() = command(d->object<Class>());
    };

    auto connection = QObject::connect(object, signal, f);

    connections.push_back(connection);
}


ObjectWrapperPrivate::~ObjectWrapperPrivate()
{
    for (auto &&c : connections) {
        QObject::disconnect(c);
    }
    ObjectShadowDataRepository::instance()->m_objectToWrapperPrivateMap.remove(m_cache->object());
}

// === ObjectHandle ===
template<typename T>
ObjectHandle<T>::ObjectHandle(std::shared_ptr<ObjectWrapperPrivate> d)
    : m_d { std::move(d) }
{}

template<typename T>
ObjectHandle<T>::operator bool() const
{
    return Probe::instance()->isValidObject(m_d.object());
}

template<typename T>
ObjectHandle<T>::operator T*() const
{
    return m_d.object();
}

template<typename T>
const ObjectWrapper<T> *ObjectHandle<T>::operator->() const
{
    return &m_d;
}

template<typename T>
ObjectWrapper<T> *ObjectHandle<T>::operator->()
{
    return &m_d;
}

template<typename T>
const ObjectWrapper<T> &ObjectHandle<T>::operator*() const
{
    return m_d;
}

template<typename T>
ObjectWrapper<T> &ObjectHandle<T>::operator*()
{
    return m_d;
}

template<typename T>
T *ObjectHandle<T>::object() const
{
    return m_d.object();
}
template<typename T>
T *ObjectHandle<T>::data() const
{
    return m_d.object();
}
template<typename T>
ObjectId ObjectHandle<T>::objectId() const
{
    return ObjectId {m_d.object()};
}



template<typename T>
template<typename Func, typename ...Args>
auto ObjectHandle<T>::call(Func &&f, Args &&...args) -> std::future<decltype(std::declval<T*>()->*f(args...))>
{
    if (!Probe::instance()->isValidObject(m_d.object())) {
        return {};
    }

    std::promise<decltype(m_d.object()->*f(args...))> p;
    auto future = p.get_future();
    if (m_d.object()->thread == QThread::currentThread()) {
        p.set_value(m_d.object()->*f(args...));
    } else {
        T *ptr = m_d->object;
        QMetaObject::invokeMethod(m_d.object(), [p, ptr, f, args...]() {
            p.set_value(ptr->*f(args...));
        }, Qt::QueuedConnection);
    }
    return future;
}

template<typename T>
void ObjectHandle<T>::refresh()
{
    m_d.d_ptr()->template cache<T>()->update(m_d.d_ptr());
}


template<typename T>
MetaObject *ObjectHandle<T>::staticMetaObject()
{
    return decltype(m_d)::staticMetaObject();
}

// === ObjectView ===

template<typename T>
ObjectView<T>::ObjectView(std::weak_ptr<ObjectWrapperPrivate> controlBlock)
: d(std::move(controlBlock))
{}

template<typename T>
ObjectView<T>::operator bool() const
{
    return !d.expired() && Probe::instance()->isValidObject(d.lock()->template object<QObject>()); // FIXME we should not need to lock this just to do a null check
}

template<typename T>
ObjectHandle<T> ObjectView<T>::lock() const
{
    return ObjectHandle<T> { d.lock() };
}
template<typename T>
ObjectView<T> ObjectView<T>::nullhandle()
{
    return ObjectView<T> { std::weak_ptr<ObjectWrapperPrivate>{} };
}

template<typename T>
const ObjectHandle<T> ObjectView<T>::operator->() const
{
    return lock();
}

template<typename T>
const ObjectWrapper<T> &ObjectView<T>::operator*() const
{
    return *lock();
}

template<typename T>
ObjectWrapper<T> &ObjectView<T>::operator*()
{
    return *lock();
}

template<typename T>
T *ObjectView<T>::object() const
{
    return d.lock()->template object<T>();
}
template<typename T>
ObjectId ObjectView<T>::objectId() const
{
    return ObjectId {d.lock()->template object<T>()};
}

// === ObjectShadowDataRepository ===

ObjectShadowDataRepository *ObjectShadowDataRepository::instance()
{
//     return Probe::instance()->objectShadowDataRepository();
    static ObjectShadowDataRepository *self = new ObjectShadowDataRepository();
    return self;
}

template<typename Class>
ObjectHandle<Class> ObjectShadowDataRepository::handleForObject(Class *obj)
{
    if (!obj) {
        return ObjectHandle<Class>{};
    }

    auto self = instance();
    std::shared_ptr<ObjectWrapperPrivate> d {};

    if (self->m_objectToWrapperPrivateMap.contains(obj)) {
        d = self->m_objectToWrapperPrivateMap.value(obj).lock();
    } else {
        d = ObjectWrapperPrivate::create(obj);
        self->m_objectToWrapperPrivateMap.insert(obj, std::weak_ptr<ObjectWrapperPrivate> { d });

        IF_CONSTEXPR (!cachingDisabled<ObjectWrapper<Class>>::value) {
            d->cache<Class>()->update(d.get());
        }
    }

    return ObjectHandle<Class> { std::move(d) };
}

template<typename Class>
ObjectView<Class> ObjectShadowDataRepository::viewForObject(Class *obj)
{
    if (!obj) {
        return ObjectView<Class> {};
    }

    auto self = instance();

    Q_ASSERT_X(self->m_objectToWrapperPrivateMap.contains(obj), "viewForObject", "Obtaining a weak handle requires a (strong) handle to already exist.");

    auto controlBasePtr = self->m_objectToWrapperPrivateMap.value(obj).lock();
    std::weak_ptr<ObjectWrapperPrivate> controlPtr =
        std::static_pointer_cast<ObjectWrapperPrivate>(controlBasePtr);
    return ObjectView<Class> { std::move(controlPtr) };
}

template<int flags, typename T>
auto wrap(T &&value) -> typename std::enable_if<!isSpecialized<ObjectWrapper<T>>::value, T>::type
{
    return std::forward<T>(value);
}
template<int flags, typename T>
auto wrap(T *object) -> second_t<typename ObjectWrapper<T>::value_type, typename std::enable_if<(flags & NonOwningPointer) != 0, ObjectView<T>>::type>
{
    return ObjectShadowDataRepository::viewForObject(object);
}
template<int flags, typename T>
auto wrap(T *object) -> second_t<typename ObjectWrapper<T>::value_type, typename std::enable_if<(flags & OwningPointer) != 0, ObjectHandle<T>>::type>
{
    return ObjectShadowDataRepository::handleForObject(object);
}
template<int flags, typename T>
auto wrap(const QList<T*> &list) -> second_t<typename ObjectWrapper<T>::value_type, QList<ObjectView<T>>>
{
    QList<ObjectView<T>> handleList;
    std::transform(list.cBegin(), list.cEnd(), handleList.begin(), [](T *t) { return ObjectShadowDataRepository::viewForObject(t); });
    return handleList;
}
template<int flags, typename T>
auto wrap(QVector<T*> list) -> second_t<typename ObjectWrapper<T>::value_type, typename std::enable_if<(flags & NonOwningPointer) != 0, QVector<ObjectView<T>>>::type>
{
    QVector<ObjectView<T>> handleList;
    handleList.reserve(list.size());
    for (T *t : qAsConst(list)) {
        handleList.push_back(ObjectShadowDataRepository::viewForObject(t));
    }
    //     std::transform(list.cbegin(), list.cend(), handleList.begin(), [](T *t) { return ObjectView<T> { t }; });
    return handleList;
}
template<int flags, typename T>
auto wrap(QVector<T*> list) -> second_t<typename ObjectWrapper<T>::value_type, typename std::enable_if<(flags & OwningPointer) != 0, QVector<ObjectHandle<T>>>::type>
{
    QVector<ObjectHandle<T>> handleList;
    handleList.reserve(list.size());
    for (T *t : qAsConst(list)) {
        handleList.push_back(ObjectShadowDataRepository::handleForObject(t));
    }
    //     std::transform(list.cbegin(), list.cend(), handleList.begin(), [](T *t) { return ObjectView<T> { t }; });
    return handleList;
}

template<typename T, typename ...Args>
auto unwrap(T && value, Args &&...dummy) -> decltype(std::forward<T>(value)) // Actually this is only supposed to support one argument, the variadic argument list is only to declare this as the fallback option.
{
    return std::forward<T>(value);
}
template<typename T>
T *unwrap(const ObjectView<T> &object)
{
    return object.object();
}
template<typename T>
T *unwrap(const ObjectHandle<T> &object)
{
    return object.object();
}
template<typename T>
T *unwrap(ObjectView<T> &object)
{
    return object.object();
}
template<typename T>
T *unwrap(ObjectHandle<T> &object)
{
    return object.object();
}
template<typename T>
auto unwrap(QList<ObjectView<T>> list) -> QList<T*>
{
    QList<T*> unwrappedList;
    for (T *t : qAsConst(list)) {
        unwrappedList.push_back(t.object());
    }
    return unwrappedList;
}
template<typename T>
auto unwrap(QList<ObjectHandle<T>> list) -> QList<T*>
{
    QList<T*> unwrappedList;
    for (T *t : qAsConst(list)) {
        unwrappedList.push_back(t.object());
    }
    return unwrappedList;
}
template<typename T>
auto unwrap(QVector<ObjectView<T>> list) -> QVector<T*>
{
    QVector<T*> unwrappedList;
    for (T *t : qAsConst(list)) {
        unwrappedList.push_back(t.object());
    }
    return unwrappedList;
}
template<typename T>
auto unwrap(QVector<ObjectHandle<T>> list) -> QVector<T*>
{
    QVector<T*> unwrappedList;
    for (T *t : qAsConst(list)) {
        unwrappedList.push_back(t.object());
    }
    return unwrappedList;
}

}


// TODO: check updating model, Check threading model, lazy-properties, inheritance-trees
// TODO: Look at Event-monitor, look at Bindings und Signal-Slot connections

// TODO: Do we actually need shared_ptr + weak_ptr or is unique_ptr + raw pointer enough?

#endif // GAMMARAY_OBJECTHANDLE_H
