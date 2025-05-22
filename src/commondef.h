#pragma once
#define DECLARE_DPTR(Class) \
struct  Class##Private; \
Q_DECLARE_PRIVATE(Class); \
QScopedPointer<Class##Private> d_ptr
