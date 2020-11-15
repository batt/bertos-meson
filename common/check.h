#ifndef CHECK_H
#define CHECK_H

#include "config.h"
#include <cfg/for.h>

struct CheckEntry;
typedef int (*check_cb)(const struct CheckEntry *);

typedef struct CheckEntry
{
	int *limit;
	int error;
	check_cb check;
	void *arg;
} CheckEntry;

#define CHECK_DO(e) (e)->check(e)

#define CHECK(name, error, min, max, default_val, check_function, arg) \
	(CONT, name, error, min, max, default_val, check_function, arg)

#define CHECK_END(name, error, min, max, default_val, check_function, arg) \
	(END, name, error, min, max, default_val, check_function, arg)

#define CHECK_NAME(name) __limit_##name

#define DECLARE_CHECK_ENTRY(cont, name, error, min, max, default_val, check_function, arg) \
	static int CHECK_NAME(name) = default_val;                                             \
	static const CheckEntry __check_##name = {&CHECK_NAME(name), error, check_function, (void *)arg};

#define DECLARE_CHECK_ENTRY_PTR(cont, name, error, min, max, default_val, check_function, arg) \
	&__check_##name,

#define EXPAND_CHECK(cont, ...)           \
	IDENTITY(PP_CAT(EXPAND_CHECK_, cont)) \
	(__VA_ARGS__)

#define EXPAND_CHECK_CONT(name, error, min, max, default_val, check_function, arg) \
	CONF_INT_NODECLARE(name, CHECK_NAME(name), min, max, default_val),

#define EXPAND_CHECK_END(name, error, min, max, default_val, check_function, arg) \
	CONF_INT_NODECLARE(name, CHECK_NAME(name), min, max, default_val)

#define DECLARE_CHECK(mod_name, table_name, ...)        \
	FOR(DECLARE_CHECK_ENTRY, __VA_ARGS__)               \
	static const CheckEntry *const table_name[] =       \
	    {                                               \
	        FOR(DECLARE_CHECK_ENTRY_PTR, __VA_ARGS__)}; \
	DECLARE_CONF(mod_name, NULL, FOR(EXPAND_CHECK, __VA_ARGS__));

int check_min(const CheckEntry *e);
int check_max(const CheckEntry *e);

#endif //CHECK_H
