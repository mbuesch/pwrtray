#ifndef PROBE_H_
#define PROBE_H_

#include "util.h"


typedef void * (*probefunc_t)(void);

struct probe {
	probefunc_t func;

	int __end[0] ALIGN_MAX;
};

#define DECLARE_PROBES(_type)							\
	extern const struct probe __start_probe_##_type;			\
	extern const struct probe __stop_probe_##_type

#define DEFINE_PROBE(_type, _name, _func)					\
	static const struct probe probe_##_type##_##_name			\
	__attribute__((__used__))						\
	__attribute__((__section__("probe_" stringify(_type))))			\
	= {									\
		.func	= (probefunc_t)(_func),					\
	}

#define for_each_probe(_ptr, _type)						\
	for (_ptr = &__start_probe_##_type;					\
	     _ptr < &__stop_probe_##_type;					\
	     _ptr++)

#endif /* PROBE_H_ */
