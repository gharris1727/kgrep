#include <sys/types.h>
#include <sys/module.h>
#include <sys/systm.h>
#include <sys/errno.h>
#include <sys/param.h>
#include <sys/kernel.h>
#include <sys/proc.h>
#include <sys/conf.h>
#include <sys/filedesc.h>
#include <sys/file.h>
#include <sys/sysproto.h>

#include "control.h"

    static int 
kagrep_loader(struct module *m, int what, void *arg) 
{
    int err = 0;

    switch (what) {
        case MOD_LOAD:
            err = control_load();
            break;
        case MOD_UNLOAD:
            control_unload();
            break;
        default:
            err = EOPNOTSUPP;
            break;
    }
    return(err);
}

static moduledata_t kagrep_mod = {
    "kagrep",
    kagrep_loader,
    NULL
};

DECLARE_MODULE(kagrep, kagrep_mod, SI_SUB_KLD, SI_ORDER_ANY);
