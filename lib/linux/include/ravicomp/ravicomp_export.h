
#ifndef RAVICOMP_EXPORT_H
#define RAVICOMP_EXPORT_H

#ifdef RAVICOMP_STATIC_DEFINE
#  define RAVICOMP_EXPORT
#  define RAVICOMP_NO_EXPORT
#else
#  ifndef RAVICOMP_EXPORT
#    ifdef ravicomp_EXPORTS
        /* We are building this library */
#      define RAVICOMP_EXPORT __attribute__((visibility("default")))
#    else
        /* We are using this library */
#      define RAVICOMP_EXPORT __attribute__((visibility("default")))
#    endif
#  endif

#  ifndef RAVICOMP_NO_EXPORT
#    define RAVICOMP_NO_EXPORT __attribute__((visibility("hidden")))
#  endif
#endif

#ifndef RAVICOMP_DEPRECATED
#  define RAVICOMP_DEPRECATED __attribute__ ((__deprecated__))
#endif

#ifndef RAVICOMP_DEPRECATED_EXPORT
#  define RAVICOMP_DEPRECATED_EXPORT RAVICOMP_EXPORT RAVICOMP_DEPRECATED
#endif

#ifndef RAVICOMP_DEPRECATED_NO_EXPORT
#  define RAVICOMP_DEPRECATED_NO_EXPORT RAVICOMP_NO_EXPORT RAVICOMP_DEPRECATED
#endif

#if 0 /* DEFINE_NO_DEPRECATED */
#  ifndef RAVICOMP_NO_DEPRECATED
#    define RAVICOMP_NO_DEPRECATED
#  endif
#endif

#endif /* RAVICOMP_EXPORT_H */
