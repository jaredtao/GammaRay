/*
 *  qmlsupporttest.cpp
 *
 *  This file is part of GammaRay, the Qt application inspection and
 *  manipulation tool.
 *
 *  Copyright (C) 2019 Klarälvdalens Datakonsult AB, a KDAB Group company, info@kdab.com
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

#include "baseprobetest.h"

#include <core/objectwrapper.h>

#include <QDebug>
#include <QtTest/qtest.h>
#include <QObject>
#include <QThread>
#include <QSignalSpy>


#include <QObject>
#include <QTimer>
#include <QVector>

#include <type_traits>
#include <memory>

using namespace GammaRay;

class SimpleNonQObjectTestObject {
public:
    explicit SimpleNonQObjectTestObject(int x, int y) : y(y), m_x(x) {}

    void setX(int x) { m_x = x; }

    int x() const { return m_x; }
    int y;

private:
    int m_x;
};

class NonConstGetterTestObject {
public:
    explicit NonConstGetterTestObject(int x) : m_x(x) {}

    int x() { return m_x; }

private:
    int m_x;
};

class DerivedTestObject : public SimpleNonQObjectTestObject
{
public:
    explicit DerivedTestObject(int x, int y, int z)
        : SimpleNonQObjectTestObject{x, y}, m_z{z}
    {}

    int z() const { return m_z; }
    void setZ(int z) { m_z = z; }

private:
    int m_z;

};

class TestMixin
{
public:
    explicit TestMixin(double a)
    : m_a(a)
    {}

    double a() const { return m_a; }

    void setA(double a) { m_a = a; }

private:
    double m_a;
};

class MultiInheritanceTestObject : public DerivedTestObject, public TestMixin
{

public:
    explicit MultiInheritanceTestObject(int x, int y, int z, double a, int b)
    : DerivedTestObject{x, y, z}
    , TestMixin {a}
    , m_b{b}
    {}

    int b() const { return m_b; }
    void setB(int b) { m_b = b; }

private:
    int m_b;
};

// class Foo {
//
//
//     using value_type = SimpleNonQObjectTestObject;
//     using ThisClass_t = Foo;
//
//     static constexpr int Flags = 0;
//
//     template<typename T1, typename T2>
//     static typename std::enable_if<std::is_same<T1, T2>::value>::type noop(T1 *t1) { return t1; }
//
//     template<typename T = value_type> /* Dummy parameter to make SFINAE applicable */
//     friend auto fetch_x(ThisClass_t *self, typename std::enable_if<std::is_same<T, value_type>::value && (Flags & LAMBDA_COMMAND) != 0>::type* = nullptr)
//     -> decltype(wrap<Flags>(x(static_cast<T*>(nullptr))))
//     {
//         return wrap<Flags>(x(noop<T, value_type>(self)->object));
//     }
//     template<typename PrivateClass>
//     friend auto fetch_x(ThisClass_t *self, typename std::enable_if<!std::is_same<PrivateClass, value_type>::value && (Flags & DptrMember) != 0>::type* = nullptr)
//     -> decltype(wrap<Flags>(std::declval<PrivateClass>().x()))
//     {
//         return wrap<Flags>(PrivateClass::get(self->object)->x());
//     }
//     template<typename T = value_type>
//     friend auto fetch_x(ThisClass_t *self, typename std::enable_if<std::is_same<T, value_type>::value && (Flags & Getter) != 0>::type* = nullptr)
//     -> decltype(wrap<Flags>(std::declval<T>().x()))
//     {
//         return wrap<Flags>(noop<T, value_type>(self)->object->x());
//     }
//     template<typename T = value_type>
//     friend auto fetch_x(ThisClass_t *self, typename std::enable_if<std::is_same<T, value_type>::value && (Flags & MemberVar) != 0>::type* = nullptr)
//     -> decltype(wrap<Flags>(std::declval<T>().x))
//     {
//         return wrap<Flags>(noop<T, value_type>(self)->object->x);
//     }
//
//     SimpleNonQObjectTestObject *object;
// };



class QObjectTestObjectPrivate : public QObjectPrivate
{
public:
    double hiddenVariable = -1.0/12.0;

    QString hiddenVarStr() const { return QString::number(hiddenVariable); }
};

class QObjectTestObject : public QObject {
    Q_OBJECT
    Q_PROPERTY(int x READ x WRITE setX NOTIFY xChanged)
    Q_PROPERTY(int y READ y WRITE setY NOTIFY yChanged)
    Q_PROPERTY(QTimer *t MEMBER t)

signals:
    void xChanged();
    void yChanged();

public:
    QObjectTestObject(QObjectTestObject *parent = nullptr)
        : QObject(*new QObjectTestObjectPrivate{}, parent)
        , t(new QTimer(this))
        , m_parent(parent)
    {}
    explicit QObjectTestObject(int x, int y, QObjectTestObject *parent = nullptr)
        : QObject(*new QObjectTestObjectPrivate{}, parent)
        , t(new QTimer(this))
        , m_x(x)
        , m_y(y)
        , m_parent(parent)
    {}
    virtual ~QObjectTestObject() {}
    int x() const { return m_x; }
    int y() const { return m_y; }

    QObjectTestObject *parent() const { return m_parent; }

    void setX(int x) { m_x = x; emit xChanged(); }
    void setY(int y) { m_y = y; emit yChanged(); }

    QString str() { return QStringLiteral("Hello World"); }
    QString echo(const QString &s) const { return s; }

    QTimer *t = nullptr;

    QVector<QObjectTestObject *> children() const
    {
        return m_children;
    }

    QVector<QObjectTestObject *> m_children;

private:
    int m_x = 8;
    int m_y = 10;
    QObjectTestObject *m_parent = nullptr;

    Q_DECLARE_PRIVATE(QObjectTestObject)
};

int getChildrenCount(const QObjectTestObject *obj)
{
    return obj->children().size();
}


class LinkedList {
public:
    LinkedList() = default;
    LinkedList(int i) : m_i(i) {}
    LinkedList(int i, LinkedList *next) : m_i(i), m_next(next) { next->m_prev = this; }
    ~LinkedList() { if(m_next) delete m_next; }

    int i() const { return m_i; }
    LinkedList *next() const { return m_next; }
    LinkedList *prev() const { return m_prev; }

    void setNext(LinkedList *next) { m_next = next; next->m_prev = this; }

private:
    int m_i;
    LinkedList *m_next = nullptr;
    LinkedList *m_prev = nullptr;
};


// class Worker : public QRunnable
// {
// public:
//     void run() override
//     {
//         newThreadObj.reset(new Test());
//         newThreadObj->setObjectName("newThreadObj");
// //         QObject::connect(newThreadObj.get(), &QObject::destroyed, mainThreadObj.get(), &QObject::deleteLater, Qt::DirectConnection);
//     }
//     std::unique_ptr<Test> newThreadObj;
//     std::unique_ptr<QObject> mainThreadObj;
// };

class DisabledCachingTestObject
{
public:
    int x() const
    {
        ++m_callCount;
        return m_x;
    }

    mutable int m_callCount = 0;
    int m_x = 42;
};

DECLARE_OBJECT_WRAPPER(SimpleNonQObjectTestObject,
                       RO_PROP(x, Getter)
                       RW_PROP(y, setY, MemberVar)
)
DECLARE_OBJECT_WRAPPER(NonConstGetterTestObject,
                       RO_PROP(x, Getter | NonConst)
                       CUSTOM_PROP(y, object->x() * 2.0, NonConst)
)
DECLARE_OBJECT_WRAPPER(QTimer, RO_PROP(isActive, Getter))
DECLARE_OBJECT_WRAPPER(QObjectTestObject,
                       PRIVATE_CLASS(QObjectTestObjectPrivate)
                       RO_PROP(x, Getter | QProp)
                       RW_PROP(y, setY, Getter | QProp)
                       RO_PROP(str, Getter | NonConst)
                       CUSTOM_PROP(halloDu, object->echo("Hello, you."), CustomCommand)
                       RO_PROP(t, MemberVar | OwningPointer)
                       RO_PROP(children, Getter | OwningPointer)
                       RO_PROP(parent, Getter | NonOwningPointer)
                       CUSTOM_PROP(childrenCount, getChildrenCount(object), CustomCommand)
                       RO_PROP(hiddenVariable, DptrMember)
                       RO_PROP(hiddenVarStr, DptrGetter)
)
DECLARE_OBJECT_WRAPPER(LinkedList,
                       RO_PROP(i, Getter)
                       RO_PROP(prev, Getter | NonOwningPointer)
                       RW_PROP(next, setNext, Getter | OwningPointer)
)
DECLARE_OBJECT_WRAPPER(DisabledCachingTestObject,
                       DISABLE_CACHING
                       RO_PROP(x, Getter)
)
DECLARE_OBJECT_WRAPPER_WB(DerivedTestObject, SimpleNonQObjectTestObject,
                       RW_PROP(z, setZ, Getter)
)
DECLARE_OBJECT_WRAPPER(TestMixin,
                          RW_PROP(a, setA, Getter)
)
DECLARE_OBJECT_WRAPPER_WB2(MultiInheritanceTestObject, DerivedTestObject, TestMixin,
                          RW_PROP(b, setB, Getter)
)
namespace GammaRay {

class ObjectWrapperTest : public BaseProbeTest
{
    Q_OBJECT
private slots:
    virtual void init()
    {
        createProbe();
    }

    void initTestCase()
    {
//         ObjectShadowDataRepository::instance()->m_objectToWrapperPrivateMap.clear(); // FIXME this should not be necessary!
    }

    void testBasics()
    {
        QObjectTestObject t;
        t.m_children = QVector<QObjectTestObject *> { new QObjectTestObject {1, 2, &t}, new QObjectTestObject {3, 4, &t} };
        ObjectHandle<QObjectTestObject> w = ObjectShadowDataRepository::handleForObject(&t);
        //     error<decltype(w->children())>();

        static_assert(std::is_same<pimplClass_t<ObjectWrapper<QObjectTestObject>>, QObjectTestObjectPrivate>::value, "Something is wrong with pimplClass_t...");

        QCOMPARE(w->x(), t.x());
        QCOMPARE(w->y(), t.y());
        QCOMPARE(w->str(), t.str());
        QCOMPARE(w->halloDu(), QStringLiteral("Hello, you."));
        QCOMPARE(w->t()->isActive(), t.t->isActive());
        auto d = static_cast<QObjectTestObjectPrivate*>(QObjectPrivate::get(&t));
        QCOMPARE(w->hiddenVariable(), d->hiddenVariable);
        QCOMPARE(w->hiddenVarStr(), d->hiddenVarStr());

        t.setX(16);
        t.setY(20);

        QCOMPARE(w->x(), t.x());
        QCOMPARE(w->y(), t.y());
        QCOMPARE(w->str(), t.str());
        QCOMPARE(w->halloDu(), QStringLiteral("Hello, you."));
        QCOMPARE(w->t()->isActive(), t.t->isActive());

        static_assert(std::is_same<decltype(w->t()), ObjectHandle<QTimer>>::value,
                      "Something broke with the wrapping of objects into ObjectHandles...");
        static_assert(std::is_same<typename decltype(w->children())::value_type, ObjectHandle<QObjectTestObject >>::value,
                      "Something broke with the wrapping of object lists into lists of ObjectHandles...");

        for (auto x : w->children()) {
            QCOMPARE(x->parent()->d, w->d);
            QCOMPARE(x->x(), x.object()->x());
            QCOMPARE(x->y(), x.object()->y());
            QCOMPARE(x->str(), x.object()->str());
            QCOMPARE(x->halloDu(), QStringLiteral("Hello, you."));
            QCOMPARE(x->t()->isActive(), x.object()->t->isActive());
        }
    }

    void testCleanup()
    {
        ObjectShadowDataRepository::instance()->m_objectToWrapperPrivateMap.clear();
        {
            QObjectTestObject t;
            t.m_children = QVector<QObjectTestObject *> { new QObjectTestObject {1, 2, &t}, new QObjectTestObject {3, 4, &t} };
            ObjectHandle<QObjectTestObject> w = ObjectShadowDataRepository::handleForObject(&t);
            QCOMPARE(ObjectShadowDataRepository::instance()->m_objectToWrapperPrivateMap.size(), 6); // test object with two children, every test object has a qtimer-child.
        }
        QCOMPARE(ObjectShadowDataRepository::instance()->m_objectToWrapperPrivateMap.size(), 0);
    }

// SKIP: This feature has been removed. It's now simply forbidden (and asserted) to create an ObjectHandle from the wrong thread.
//     void testThreadBoundaries()
//     {
// //         auto task = std::unique_ptr<Worker>(new Worker());
// //         task->setAutoDelete(false);
// //         QThreadPool::globalInstance()->start(task.get());
//
//         ObjectShadowDataRepository::instance()->m_objectToWrapperPrivateMap.clear(); // FIXME this should not be necessary!
//         QThread workerThread;
//         workerThread.start();
//         QObjectTestObject t;
//         t.m_children = QVector<QObjectTestObject *> { new QObjectTestObject {1, 2, &t}, new QObjectTestObject {3, 4, &t} };
//         t.moveToThread(&workerThread);
//
//         ObjectHandle<QObjectTestObject> w { &t };
//         //     error<decltype(w->children())>();
//
//         QCOMPARE(w->x(), t.x());
//         QCOMPARE(w->y(), t.y());
//         QCOMPARE(w->str(), t.str());
//         QCOMPARE(w->halloDu(), QStringLiteral("Hello, you."));
//         QCOMPARE(w->t()->isActive(), t.t->isActive());
//
//         t.setX(16);
//         t.setY(20);
//
//         QCOMPARE(w->x(), t.x());
//         QCOMPARE(w->y(), t.y());
//         QCOMPARE(w->str(), t.str());
//         QCOMPARE(w->halloDu(), QStringLiteral("Hello, you."));
//         QCOMPARE(w->t()->isActive(), t.t->isActive());
//
//         static_assert(std::is_same<typename decltype(w->children())::value_type, ObjectHandle<QObjectTestObject >>::value,
//                       "Something broke with the wrapping of object lists into lists of ObjectHandles...");
//
//         for (auto x : w->children()) {
//             QCOMPARE(x->parent()->d, w->d);
//             QCOMPARE(x->x(), x.object()->x());
//             QCOMPARE(x->y(), x.object()->y());
//             QCOMPARE(x->str(), x.object()->str());
//             QCOMPARE(x->halloDu(), QStringLiteral("Hello, you."));
//             QCOMPARE(x->t()->isActive(), x.object()->t->isActive());
//         }
//
//         workerThread.quit();
//         workerThread.wait();
//     }

    void testSelfReference()
    {
        ObjectShadowDataRepository::instance()->m_objectToWrapperPrivateMap.clear();
        {
            LinkedList ll { 5, new LinkedList(6) };
            ObjectHandle<LinkedList> l = ObjectShadowDataRepository::handleForObject(&ll);

            QCOMPARE(ObjectShadowDataRepository::instance()->m_objectToWrapperPrivateMap.size(), 2);

            QVERIFY(l.object());
            QVERIFY(l->d);
            QVERIFY(ll.next()->prev());
            QVERIFY(l->next()->d != l->d);
            QVERIFY(l->next().object());
            QVERIFY(l->next()->prev().object());
            QCOMPARE(l->next()->prev()->d, l->d);
            QCOMPARE(l->next()->i(), ll.next()->i());
            QCOMPARE(l->next()->prev()->i(), ll.i());
            QCOMPARE(l->next()->prev()->next()->i(), ll.next()->i());
            QCOMPARE(l->next()->prev()->next()->prev()->i(), ll.i());
            QCOMPARE(l->next()->prev()->next()->prev()->next()->i(), ll.next()->i());
        }
        QCOMPARE(ObjectShadowDataRepository::instance()->m_objectToWrapperPrivateMap.size(), 0);
    }

    void testMoveHandle()
    {
        QObjectTestObject t;
        ObjectHandle<QObjectTestObject> w;

        {
            auto wptr = ObjectShadowDataRepository::handleForObject(&t); // FIXME was `new ObjectHandle<QObjectTestObject> {&t}`. Is the test still correct?
            w = wptr; // copy
        } // delete wptr

        QCOMPARE(w->x(), t.x());
        QCOMPARE(w->y(), t.y());
        QCOMPARE(w->str(), t.str());
        QCOMPARE(w->halloDu(), QStringLiteral("Hello, you."));
        QCOMPARE(w->t()->isActive(), t.t->isActive());

        t.setX(16);
        t.setY(20);

        QCOMPARE(w->x(), t.x());
        QCOMPARE(w->y(), t.y());
        QCOMPARE(w->str(), t.str());
        QCOMPARE(w->halloDu(), QStringLiteral("Hello, you."));
        QCOMPARE(w->t()->isActive(), t.t->isActive());
    }

    void testNonQObject()
    {
        SimpleNonQObjectTestObject t {1, 2};
        ObjectHandle<SimpleNonQObjectTestObject> w = ObjectShadowDataRepository::handleForObject(&t);

        QCOMPARE(w->x(), t.x());
        QCOMPARE(w->y(), t.y);

        t.setX(16);
        t.y = 20;

        w.refresh();

        QCOMPARE(w->x(), t.x());
        QCOMPARE(w->y(), t.y);
    }

    void testNonConstGetter()
    {
        NonConstGetterTestObject t {1};
        ObjectHandle<NonConstGetterTestObject> w = ObjectShadowDataRepository::handleForObject(&t);

        QCOMPARE(w->x(), t.x());
        QCOMPARE(w->y(), t.x() * 2.0);
    }

    void testCachingDisabled()
    {
        DisabledCachingTestObject t;
        ObjectHandle<DisabledCachingTestObject> w = ObjectShadowDataRepository::handleForObject(&t);

        static_assert(cachingDisabled<ObjectWrapper<DisabledCachingTestObject>>::value,
                      "cachingDisabled is not reported for test object.");

        QCOMPARE(t.m_callCount, 0);
        QCOMPARE(w->x(), 42);
        QCOMPARE(t.m_callCount, 1);

        t.m_x = 21;
        QCOMPARE(w->x(), 21);
        QCOMPARE(t.m_callCount, 2);
    }

    void testMetaObject()
    {
        SimpleNonQObjectTestObject t {1, 2};
        ObjectHandle<SimpleNonQObjectTestObject> w = ObjectShadowDataRepository::handleForObject(&t);
        auto mo = ObjectHandle<SimpleNonQObjectTestObject>::staticMetaObject();

        QCOMPARE(mo->className(), QStringLiteral("SimpleNonQObjectTestObject"));
        QCOMPARE(mo->propertyCount(), 2);
        QCOMPARE(mo->propertyAt(0)->name(), "y"); // TODO is it a problem that the meta object lists the properties in inverted order?
        QCOMPARE(mo->propertyAt(0)->typeName(), "int");
        QCOMPARE(mo->propertyAt(0)->value(&*w), 2); // FIXME Fix the getter-API for accessing values through ObjectHandle-MetaObjects
        QCOMPARE(mo->propertyAt(1)->name(), "x");
        QCOMPARE(mo->propertyAt(1)->typeName(), "int");
        QCOMPARE(mo->propertyAt(1)->value(&*w), 1);

        t.setX(16);
        t.y = 20;

        QCOMPARE(mo->propertyAt(0)->value(&*w), 2);
        QCOMPARE(mo->propertyAt(1)->value(&*w), 1);
        w.refresh();
        QCOMPARE(mo->propertyAt(0)->value(&*w), 20);
        QCOMPARE(mo->propertyAt(1)->value(&*w), 16);

    }

    void testWriting()
    {
        SimpleNonQObjectTestObject t {1, 2};
        ObjectHandle<SimpleNonQObjectTestObject> w = ObjectShadowDataRepository::handleForObject(&t);

        QCOMPARE(w->y(), t.y);
        w->setY(20);
        QCOMPARE(w->y(), 20);
        QCOMPARE(w->y(), t.y);
    }


    void testWritingUnwrapping()
    {
        LinkedList ll { 5, new LinkedList(6) };
        ObjectHandle<LinkedList> w = ObjectShadowDataRepository::handleForObject(&ll);
        ObjectHandle<LinkedList> w2 = ObjectShadowDataRepository::handleForObject(new LinkedList { 7, new LinkedList(8) });

        static_assert(std::is_same<decltype(unwrap(w2)), LinkedList *>::value);

        QCOMPARE(w->next()->next(), ObjectHandle<LinkedList>{});
        w->next()->setNext(w2);
        QCOMPARE(w->next()->next(), w2);
    }


    void testSingleInheritance()
    {
        DerivedTestObject t {1, 2, 3};
        ObjectHandle<DerivedTestObject> w = ObjectShadowDataRepository::handleForObject(&t);

        QCOMPARE(w->x(), t.x());
        QCOMPARE(w->y(), t.y);
        QCOMPARE(w->z(), t.z());

        ObjectHandle<SimpleNonQObjectTestObject> v = w;

        QCOMPARE(v->x(), t.x());
        QCOMPARE(v->y(), t.y);
    }


    void testMultipleInheritance()
    {
        MultiInheritanceTestObject t {1, 2, 3, 4.0, 5};
        ObjectHandle<MultiInheritanceTestObject> w = ObjectShadowDataRepository::handleForObject(&t);

        QCOMPARE(w->x(), t.x());
        QCOMPARE(w->y(), t.y);
        QCOMPARE(w->z(), t.z());
        QCOMPARE(w->a(), t.a());
        QCOMPARE(w->b(), t.b());

        ObjectHandle<SimpleNonQObjectTestObject> v = w;

        QCOMPARE(v.object(), static_cast<SimpleNonQObjectTestObject*>(&t));
        QCOMPARE(v->x(), t.x());
        QCOMPARE(v->y(), t.y);

        ObjectHandle<TestMixin> u = w;

        QCOMPARE(u.object(), static_cast<TestMixin*>(&t));
        QCOMPARE(u->a(), t.a());

        // Test MetaObject
        auto mo = ObjectHandle<MultiInheritanceTestObject>::staticMetaObject();

        QCOMPARE(mo->className(), QStringLiteral("MultiInheritanceTestObject"));
        QCOMPARE(mo->propertyCount(), 5);
        QCOMPARE(mo->propertyAt(0)->name(), "y"); // TODO is it a problem that the meta object lists the properties in inverted order?
        QCOMPARE(mo->propertyAt(0)->typeName(), "int");
        QCOMPARE(mo->propertyAt(0)->value(&*w), 2); // FIXME Fix the getter-API for accessing values through ObjectHandle-MetaObjects
        QCOMPARE(mo->propertyAt(1)->name(), "x");
        QCOMPARE(mo->propertyAt(1)->typeName(), "int");
        QCOMPARE(mo->propertyAt(1)->value(&*w), 1);

        QCOMPARE(mo->propertyAt(2)->name(), "z");
        QCOMPARE(mo->propertyAt(2)->typeName(), "int");
        QCOMPARE(mo->propertyAt(2)->value(&*w), 3);
        QCOMPARE(mo->propertyAt(3)->name(), "a");
        QCOMPARE(mo->propertyAt(3)->typeName(), "double");
        QCOMPARE(mo->propertyAt(3)->value(&*w), 4.0);
        QCOMPARE(mo->propertyAt(4)->name(), "b");
        QCOMPARE(mo->propertyAt(4)->typeName(), "int");
        QCOMPARE(mo->propertyAt(4)->value(&*w), 5);

        QVERIFY(mo->inherits(QStringLiteral("DerivedTestObject")));
        auto mo_DTO = mo->superClass(0);
        QCOMPARE(mo_DTO->className(), QStringLiteral("DerivedTestObject"));

        auto mo_SNQOTO = mo_DTO->superClass(0);
        QCOMPARE(mo_SNQOTO->className(), QStringLiteral("SimpleNonQObjectTestObject"));
        QCOMPARE(mo_SNQOTO->propertyAt(0)->name(), "y");
        QCOMPARE(mo_SNQOTO->propertyAt(0)->typeName(), "int");
        QCOMPARE(mo_SNQOTO->propertyAt(0)->value(&*w), 2);
        QCOMPARE(mo_SNQOTO->propertyAt(1)->name(), "x");
        QCOMPARE(mo_SNQOTO->propertyAt(1)->typeName(), "int");
        QCOMPARE(mo_SNQOTO->propertyAt(1)->value(&*w), 1);

        auto mo_tm = mo->superClass(1);
        QCOMPARE(mo_tm->className(), QStringLiteral("TestMixin"));
        QCOMPARE(mo_tm->propertyAt(0)->name(), "a");
        QCOMPARE(mo_tm->propertyAt(0)->typeName(), "double");
        QCOMPARE(mo_tm->propertyAt(0)->value(&*w), 4.0);

        // Test writing

        w->setY(6);
        w->setZ(7);
        u->setA(8.3);
        w->setB(9);


        QCOMPARE(v->y(), 6);
        QCOMPARE(v->y(), t.y);
        QCOMPARE(w->z(), 7);
        QCOMPARE(w->z(), t.z());
        QCOMPARE(w->a(), 8.3);
        QCOMPARE(w->a(), t.a());
        QCOMPARE(w->b(), 9);
        QCOMPARE(w->b(), t.b());

    }
};

}


QTEST_MAIN(GammaRay::ObjectWrapperTest)

#include "objectwrappertest.moc"
