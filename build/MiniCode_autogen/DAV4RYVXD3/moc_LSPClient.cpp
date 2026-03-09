/****************************************************************************
** Meta object code from reading C++ file 'LSPClient.hpp'
**
** Created by: The Qt Meta Object Compiler version 68 (Qt 6.4.2)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include <memory>
#include "../../../src/lsp/LSPClient.hpp"
#include <QtCore/qmetatype.h>
#include <QtCore/QList>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'LSPClient.hpp' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 68
#error "This file was generated using the moc from 6.4.2. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

#ifndef Q_CONSTINIT
#define Q_CONSTINIT
#endif

QT_BEGIN_MOC_NAMESPACE
QT_WARNING_PUSH
QT_WARNING_DISABLE_DEPRECATED
namespace {
struct qt_meta_stringdata_minicode__LSPClient_t {
    uint offsetsAndSizes[50];
    char stringdata0[20];
    char stringdata1[20];
    char stringdata2[1];
    char stringdata3[4];
    char stringdata4[18];
    char stringdata5[12];
    char stringdata6[19];
    char stringdata7[22];
    char stringdata8[6];
    char stringdata9[19];
    char stringdata10[5];
    char stringdata11[10];
    char stringdata12[14];
    char stringdata13[8];
    char stringdata14[12];
    char stringdata15[14];
    char stringdata16[14];
    char stringdata17[12];
    char stringdata18[15];
    char stringdata19[23];
    char stringdata20[6];
    char stringdata21[18];
    char stringdata22[9];
    char stringdata23[21];
    char stringdata24[7];
};
#define QT_MOC_LITERAL(ofs, len) \
    uint(sizeof(qt_meta_stringdata_minicode__LSPClient_t::offsetsAndSizes) + ofs), len 
Q_CONSTINIT static const qt_meta_stringdata_minicode__LSPClient_t qt_meta_stringdata_minicode__LSPClient = {
    {
        QT_MOC_LITERAL(0, 19),  // "minicode::LSPClient"
        QT_MOC_LITERAL(20, 19),  // "diagnosticsReceived"
        QT_MOC_LITERAL(40, 0),  // ""
        QT_MOC_LITERAL(41, 3),  // "uri"
        QT_MOC_LITERAL(45, 17),  // "QList<Diagnostic>"
        QT_MOC_LITERAL(63, 11),  // "diagnostics"
        QT_MOC_LITERAL(75, 18),  // "completionReceived"
        QT_MOC_LITERAL(94, 21),  // "QList<CompletionItem>"
        QT_MOC_LITERAL(116, 5),  // "items"
        QT_MOC_LITERAL(122, 18),  // "definitionReceived"
        QT_MOC_LITERAL(141, 4),  // "line"
        QT_MOC_LITERAL(146, 9),  // "character"
        QT_MOC_LITERAL(156, 13),  // "hoverReceived"
        QT_MOC_LITERAL(170, 7),  // "message"
        QT_MOC_LITERAL(178, 11),  // "serverError"
        QT_MOC_LITERAL(190, 13),  // "serverStarted"
        QT_MOC_LITERAL(204, 13),  // "serverStopped"
        QT_MOC_LITERAL(218, 11),  // "onReadyRead"
        QT_MOC_LITERAL(230, 14),  // "onProcessError"
        QT_MOC_LITERAL(245, 22),  // "QProcess::ProcessError"
        QT_MOC_LITERAL(268, 5),  // "error"
        QT_MOC_LITERAL(274, 17),  // "onProcessFinished"
        QT_MOC_LITERAL(292, 8),  // "exitCode"
        QT_MOC_LITERAL(301, 20),  // "QProcess::ExitStatus"
        QT_MOC_LITERAL(322, 6)   // "status"
    },
    "minicode::LSPClient",
    "diagnosticsReceived",
    "",
    "uri",
    "QList<Diagnostic>",
    "diagnostics",
    "completionReceived",
    "QList<CompletionItem>",
    "items",
    "definitionReceived",
    "line",
    "character",
    "hoverReceived",
    "message",
    "serverError",
    "serverStarted",
    "serverStopped",
    "onReadyRead",
    "onProcessError",
    "QProcess::ProcessError",
    "error",
    "onProcessFinished",
    "exitCode",
    "QProcess::ExitStatus",
    "status"
};
#undef QT_MOC_LITERAL
} // unnamed namespace

Q_CONSTINIT static const uint qt_meta_data_minicode__LSPClient[] = {

 // content:
      10,       // revision
       0,       // classname
       0,    0, // classinfo
      10,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       7,       // signalCount

 // signals: name, argc, parameters, tag, flags, initial metatype offsets
       1,    2,   74,    2, 0x06,    1 /* Public */,
       6,    1,   79,    2, 0x06,    4 /* Public */,
       9,    3,   82,    2, 0x06,    6 /* Public */,
      12,    1,   89,    2, 0x06,   10 /* Public */,
      14,    1,   92,    2, 0x06,   12 /* Public */,
      15,    0,   95,    2, 0x06,   14 /* Public */,
      16,    0,   96,    2, 0x06,   15 /* Public */,

 // slots: name, argc, parameters, tag, flags, initial metatype offsets
      17,    0,   97,    2, 0x08,   16 /* Private */,
      18,    1,   98,    2, 0x08,   17 /* Private */,
      21,    2,  101,    2, 0x08,   19 /* Private */,

 // signals: parameters
    QMetaType::Void, QMetaType::QString, 0x80000000 | 4,    3,    5,
    QMetaType::Void, 0x80000000 | 7,    8,
    QMetaType::Void, QMetaType::QString, QMetaType::Int, QMetaType::Int,    3,   10,   11,
    QMetaType::Void, QMetaType::QString,   13,
    QMetaType::Void, QMetaType::QString,   13,
    QMetaType::Void,
    QMetaType::Void,

 // slots: parameters
    QMetaType::Void,
    QMetaType::Void, 0x80000000 | 19,   20,
    QMetaType::Void, QMetaType::Int, 0x80000000 | 23,   22,   24,

       0        // eod
};

Q_CONSTINIT const QMetaObject minicode::LSPClient::staticMetaObject = { {
    QMetaObject::SuperData::link<QObject::staticMetaObject>(),
    qt_meta_stringdata_minicode__LSPClient.offsetsAndSizes,
    qt_meta_data_minicode__LSPClient,
    qt_static_metacall,
    nullptr,
    qt_incomplete_metaTypeArray<qt_meta_stringdata_minicode__LSPClient_t,
        // Q_OBJECT / Q_GADGET
        QtPrivate::TypeAndForceComplete<LSPClient, std::true_type>,
        // method 'diagnosticsReceived'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QString &, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QVector<Diagnostic> &, std::false_type>,
        // method 'completionReceived'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QVector<CompletionItem> &, std::false_type>,
        // method 'definitionReceived'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QString &, std::false_type>,
        QtPrivate::TypeAndForceComplete<int, std::false_type>,
        QtPrivate::TypeAndForceComplete<int, std::false_type>,
        // method 'hoverReceived'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QString &, std::false_type>,
        // method 'serverError'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QString &, std::false_type>,
        // method 'serverStarted'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'serverStopped'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'onReadyRead'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'onProcessError'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<QProcess::ProcessError, std::false_type>,
        // method 'onProcessFinished'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<int, std::false_type>,
        QtPrivate::TypeAndForceComplete<QProcess::ExitStatus, std::false_type>
    >,
    nullptr
} };

void minicode::LSPClient::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        auto *_t = static_cast<LSPClient *>(_o);
        (void)_t;
        switch (_id) {
        case 0: _t->diagnosticsReceived((*reinterpret_cast< std::add_pointer_t<QString>>(_a[1])),(*reinterpret_cast< std::add_pointer_t<QList<Diagnostic>>>(_a[2]))); break;
        case 1: _t->completionReceived((*reinterpret_cast< std::add_pointer_t<QList<CompletionItem>>>(_a[1]))); break;
        case 2: _t->definitionReceived((*reinterpret_cast< std::add_pointer_t<QString>>(_a[1])),(*reinterpret_cast< std::add_pointer_t<int>>(_a[2])),(*reinterpret_cast< std::add_pointer_t<int>>(_a[3]))); break;
        case 3: _t->hoverReceived((*reinterpret_cast< std::add_pointer_t<QString>>(_a[1]))); break;
        case 4: _t->serverError((*reinterpret_cast< std::add_pointer_t<QString>>(_a[1]))); break;
        case 5: _t->serverStarted(); break;
        case 6: _t->serverStopped(); break;
        case 7: _t->onReadyRead(); break;
        case 8: _t->onProcessError((*reinterpret_cast< std::add_pointer_t<QProcess::ProcessError>>(_a[1]))); break;
        case 9: _t->onProcessFinished((*reinterpret_cast< std::add_pointer_t<int>>(_a[1])),(*reinterpret_cast< std::add_pointer_t<QProcess::ExitStatus>>(_a[2]))); break;
        default: ;
        }
    } else if (_c == QMetaObject::IndexOfMethod) {
        int *result = reinterpret_cast<int *>(_a[0]);
        {
            using _t = void (LSPClient::*)(const QString & , const QVector<Diagnostic> & );
            if (_t _q_method = &LSPClient::diagnosticsReceived; *reinterpret_cast<_t *>(_a[1]) == _q_method) {
                *result = 0;
                return;
            }
        }
        {
            using _t = void (LSPClient::*)(const QVector<CompletionItem> & );
            if (_t _q_method = &LSPClient::completionReceived; *reinterpret_cast<_t *>(_a[1]) == _q_method) {
                *result = 1;
                return;
            }
        }
        {
            using _t = void (LSPClient::*)(const QString & , int , int );
            if (_t _q_method = &LSPClient::definitionReceived; *reinterpret_cast<_t *>(_a[1]) == _q_method) {
                *result = 2;
                return;
            }
        }
        {
            using _t = void (LSPClient::*)(const QString & );
            if (_t _q_method = &LSPClient::hoverReceived; *reinterpret_cast<_t *>(_a[1]) == _q_method) {
                *result = 3;
                return;
            }
        }
        {
            using _t = void (LSPClient::*)(const QString & );
            if (_t _q_method = &LSPClient::serverError; *reinterpret_cast<_t *>(_a[1]) == _q_method) {
                *result = 4;
                return;
            }
        }
        {
            using _t = void (LSPClient::*)();
            if (_t _q_method = &LSPClient::serverStarted; *reinterpret_cast<_t *>(_a[1]) == _q_method) {
                *result = 5;
                return;
            }
        }
        {
            using _t = void (LSPClient::*)();
            if (_t _q_method = &LSPClient::serverStopped; *reinterpret_cast<_t *>(_a[1]) == _q_method) {
                *result = 6;
                return;
            }
        }
    }
}

const QMetaObject *minicode::LSPClient::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *minicode::LSPClient::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_meta_stringdata_minicode__LSPClient.stringdata0))
        return static_cast<void*>(this);
    return QObject::qt_metacast(_clname);
}

int minicode::LSPClient::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QObject::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 10)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 10;
    } else if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 10)
            *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType();
        _id -= 10;
    }
    return _id;
}

// SIGNAL 0
void minicode::LSPClient::diagnosticsReceived(const QString & _t1, const QVector<Diagnostic> & _t2)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))), const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t2))) };
    QMetaObject::activate(this, &staticMetaObject, 0, _a);
}

// SIGNAL 1
void minicode::LSPClient::completionReceived(const QVector<CompletionItem> & _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 1, _a);
}

// SIGNAL 2
void minicode::LSPClient::definitionReceived(const QString & _t1, int _t2, int _t3)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))), const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t2))), const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t3))) };
    QMetaObject::activate(this, &staticMetaObject, 2, _a);
}

// SIGNAL 3
void minicode::LSPClient::hoverReceived(const QString & _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 3, _a);
}

// SIGNAL 4
void minicode::LSPClient::serverError(const QString & _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 4, _a);
}

// SIGNAL 5
void minicode::LSPClient::serverStarted()
{
    QMetaObject::activate(this, &staticMetaObject, 5, nullptr);
}

// SIGNAL 6
void minicode::LSPClient::serverStopped()
{
    QMetaObject::activate(this, &staticMetaObject, 6, nullptr);
}
QT_WARNING_POP
QT_END_MOC_NAMESPACE
