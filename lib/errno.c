#include <sys/errno.h>

const char* strerrno(int errno) {
    switch (errno) {
        case EPERM: {
            return "EPERM";
        } break;
        case ENOENT: {
            return "ENOENT";
        } break;
        case ESRCH: {
            return "ESRCH";
        } break;
        case EINTR: {
            return "EINTR";
        } break;
        case EIO: {
            return "EIO";
        } break;
        case ENXIO: {
            return "ENXIO";
        } break;
        case E2BIG: {
            return "E2BIG";
        } break;
        case ENOEXEC: {
            return "ENOEXEC";
        } break;
        case EBADF: {
            return "EBADF";
        } break;
        case ECHILD: {
            return "ECHILD";
        } break;
        case EAGAIN: {
            return "EAGAIN";
        } break;
        case ENOMEM: {
            return "ENOMEM";
        } break;
        case EACCES: {
            return "EACCES";
        } break;
        case EFAULT: {
            return "EFAULT";
        } break;
        case ENOTBLK: {
            return "ENOTBLK";
        } break;
        case EBUSY: {
            return "EBUSY";
        } break;
        case EEXIST: {
            return "EEXIST";
        } break;
        case EXDEV: {
            return "EXDEV";
        } break;
        case ENODEV: {
            return "ENODEV";
        } break;
        case ENOTDIR: {
            return "ENOTDIR";
        } break;
        case EISDIR: {
            return "EISDIR";
        } break;
        case EINVAL: {
            return "EINVAL";
        } break;
        case ENFILE: {
            return "ENFILE";
        } break;
        case EMFILE: {
            return "EMFILE";
        } break;
        case ENOTTY: {
            return "ENOTTY";
        } break;
        case ETXTBSY: {
            return "ETXTBSY";
        } break;
        case EFBIG: {
            return "EFBIG";
        } break;
        case ENOSPC: {
            return "ENOSPC";
        } break;
        case ESPIPE: {
            return "ESPIPE";
        } break;
        case EROFS: {
            return "EROFS";
        } break;
        case EMLINK: {
            return "EMLINK";
        } break;
        case EPIPE: {
            return "EPIPE";
        } break;
        case EDOM: {
            return "EDOM";
        } break;
        case ERANGE: {
            return "ERANGE";
        } break;
        case EDEADLK: {
            return "EDEADLK";
        } break;
        case ENAMETOOLONG: {
            return "ENAMETOOLONG";
        } break;
        case ENOLCK: {
            return "ENOLCK";
        } break;
        case ENOSYS: {
            return "ENOSYS";
        } break;
        case ENOTEMPTY: {
            return "ENOTEMPTY";
        } break;
        case ELOOP: {
            return "ELOOP";
        } break;
        case ENOMSG: {
            return "ENOMSG";
        } break;
        case EIDRM: {
            return "EIDRM";
        } break;
        case ECHRNG: {
            return "ECHRNG";
        } break;
        case EL2NSYNC: {
            return "EL2NSYNC";
        } break;
        case EL3HLT: {
            return "EL3HLT";
        } break;
        case EL3RST: {
            return "EL3RST";
        } break;
        case ELNRNG: {
            return "ELNRNG";
        } break;
        case EUNATCH: {
            return "EUNATCH";
        } break;
        case ENOCSI: {
            return "ENOCSI";
        } break;
        case EL2HLT: {
            return "EL2HLT";
        } break;
        case EBADE: {
            return "EBADE";
        } break;
        case EBADR: {
            return "EBADR";
        } break;
        case EXFULL: {
            return "EXFULL";
        } break;
        case ENOANO: {
            return "ENOANO";
        } break;
        case EBADRQC: {
            return "EBADRQC";
        } break;
        case EBADSLT: {
            return "EBADSLT";
        } break;
        case EBFONT: {
            return "EBFONT";
        } break;
        case ENOSTR: {
            return "ENOSTR";
        } break;
        case ENODATA: {
            return "ENODATA";
        } break;
        case ETIME: {
            return "ETIME";
        } break;
        case ENOSR: {
            return "ENOSR";
        } break;
        case ENONET: {
            return "ENONET";
        } break;
        case ENOPKG: {
            return "ENOPKG";
        } break;
        case EREMOTE: {
            return "EREMOTE";
        } break;
        case ENOLINK: {
            return "ENOLINK";
        } break;
        case EADV: {
            return "EADV";
        } break;
        case ESRMNT: {
            return "ESRMNT";
        } break;
        case ECOMM: {
            return "ECOMM";
        } break;
        case EPROTO: {
            return "EPROTO";
        } break;
        case EMULTIHOP: {
            return "EMULTIHOP";
        } break;
        case EDOTDOT: {
            return "EDOTDOT";
        } break;
        case EBADMSG: {
            return "EBADMSG";
        } break;
        case EOVERFLOW: {
            return "EOVERFLOW";
        } break;
        case ENOTUNIQ: {
            return "ENOTUNIQ";
        } break;
        case EBADFD: {
            return "EBADFD";
        } break;
        case EREMCHG: {
            return "EREMCHG";
        } break;
        case ELIBACC: {
            return "ELIBACC";
        } break;
        case ELIBBAD: {
            return "ELIBBAD";
        } break;
        case ELIBSCN: {
            return "ELIBSCN";
        } break;
        case ELIBMAX: {
            return "ELIBMAX";
        } break;
        case ELIBEXEC: {
            return "ELIBEXEC";
        } break;
        case EILSEQ: {
            return "EILSEQ";
        } break;
        case ERESTART: {
            return "ERESTART";
        } break;
        case ESTRPIPE: {
            return "ESTRPIPE";
        } break;
        case EUSERS: {
            return "EUSERS";
        } break;
        case ENOTSOCK: {
            return "ENOTSOCK";
        } break;
        case EDESTADDRREQ: {
            return "EDESTADDRREQ";
        } break;
        case EMSGSIZE: {
            return "EMSGSIZE";
        } break;
        case EPROTOTYPE: {
            return "EPROTOTYPE";
        } break;
        case ENOPROTOOPT: {
            return "ENOPROTOOPT";
        } break;
        case EPROTONOSUPPORT: {
            return "EPROTONOSUPPORT";
        } break;
        case ESOCKTNOSUPPORT: {
            return "ESOCKTNOSUPPORT";
        } break;
        case EOPNOTSUPP: {
            return "EOPNOTSUPP";
        } break;
        case EPFNOSUPPORT: {
            return "EPFNOSUPPORT";
        } break;
        case EAFNOSUPPORT: {
            return "EAFNOSUPPORT";
        } break;
        case EADDRINUSE: {
            return "EADDRINUSE";
        } break;
        case EADDRNOTAVAIL: {
            return "EADDRNOTAVAIL";
        } break;
        case ENETDOWN: {
            return "ENETDOWN";
        } break;
        case ENETUNREACH: {
            return "ENETUNREACH";
        } break;
        case ENETRESET: {
            return "ENETRESET";
        } break;
        case ECONNABORTED: {
            return "ECONNABORTED";
        } break;
        case ECONNRESET: {
            return "ECONNRESET";
        } break;
        case ENOBUFS: {
            return "ENOBUFS";
        } break;
        case EISCONN: {
            return "EISCONN";
        } break;
        case ENOTCONN: {
            return "ENOTCONN";
        } break;
        case ESHUTDOWN: {
            return "ESHUTDOWN";
        } break;
        case ETOOMANYREFS: {
            return "ETOOMANYREFS";
        } break;
        case ETIMEDOUT: {
            return "ETIMEDOUT";
        } break;
        case ECONNREFUSED: {
            return "ECONNREFUSED";
        } break;
        case EHOSTDOWN: {
            return "EHOSTDOWN";
        } break;
        case EHOSTUNREACH: {
            return "EHOSTUNREACH";
        } break;
        case EALREADY: {
            return "EALREADY";
        } break;
        case EINPROGRESS: {
            return "EINPROGRESS";
        } break;
        case ESTALE: {
            return "ESTALE";
        } break;
        case EUCLEAN: {
            return "EUCLEAN";
        } break;
        case ENOTNAM: {
            return "ENOTNAM";
        } break;
        case ENAVAIL: {
            return "ENAVAIL";
        } break;
        case EISNAM: {
            return "EISNAM";
        } break;
        case EREMOTEIO: {
            return "EREMOTEIO";
        } break;
        case EDQUOT: {
            return "EDQUOT";
        } break;
        case ENOMEDIUM: {
            return "ENOMEDIUM";
        } break;
        case EMEDIUMTYPE: {
            return "EMEDIUMTYPE";
        } break;
        case ECANCELED: {
            return "ECANCELED";
        } break;
        case ENOKEY: {
            return "ENOKEY";
        } break;
        case EKEYEXPIRED: {
            return "EKEYEXPIRED";
        } break;
        case EKEYREVOKED: {
            return "EKEYREVOKED";
        } break;
        case EKEYREJECTED: {
            return "EKEYREJECTED";
        } break;
        case EOWNERDEAD: {
            return "EOWNERDEAD";
        } break;
        case ENOTRECOVERABLE: {
            return "ENOTRECOVERABLE";
        } break;
        case ERFKILL: {
            return "ERFKILL";
        } break;
        case EHWPOISON: {
            return "EHWPOISON";
        } break;
        default: {
            return "EUNKNOWN";
        } break;
    }
}
