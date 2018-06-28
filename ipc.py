from constants import MED_OK, MED_NO
from framework import Register, Bitmap

register = Register()

def printk(*args):
    s = Printk()
    for m in args:
        msg = str(m)
        for i in msg.split('\n'):
            s.message = i
            s.update()

@register('ipc_ctl')
def ipc_ctl(event, process, ipc_object):
    if not ipc_object:
        printk("MYSTABLE IPC_CTL CMD:{}\n".format(event.cmd))
        return MED_OK
    printk("MYSTABLE IPC_CTL id: {}, gid:{}\n".format(ipc_object.id, ipc_object.gid))
    return MED_OK

@register('ipc_associate')
def ipc_associate(event, process, ipc_object):
    printk("MYSTABLE IPC_ASSOCIATE id: {}, gid:{}\n".format(ipc_object.id, ipc_object.gid))
    return MED_OK

@register('ipc_perm')
def ipc_perm(event, process, ipc_object):
    printk("MYSTABLE IPC_PERM id: {}, gid:{}\n".format(ipc_object.id, ipc_object.gid))
    ipc_object.update()
    return MED_OK

@register('ipc_semop')
def ipc_semop(event, process, ipc_object):
    printk("MYSTABLE IPC_SEMOP id: {}, gid:{}\n".format(ipc_object.id, ipc_object.gid))
    return MED_OK

@register('ipc_shmat')
def ipc_shmat(event, process, ipc_object):
    printk("MYSTABLE IPC_SHMAT id: {}, gid:{}\n".format(ipc_object.id, ipc_object.gid))
    ipc_object.update()
    return MED_OK

@register('ipc_msgrcv')
def ipc_msgrcv(event, process, ipc_object):
    printk("MYSTABLE IPC_MSGRCV id: {}, gid:{}\n".format(ipc_object.id, ipc_object.gid))
    return MED_OK

@register('ipc_msgsnd')
def ipc_msgsnd(event, process, ipc_object):
    printk("MYSTABLE IPC_MSGSND id: {}, gid:{}\n".format(ipc_object.id, ipc_object.gid))
    return MED_OK

@register('getipc')
def ipc(event, sender):
    return MED_OK
