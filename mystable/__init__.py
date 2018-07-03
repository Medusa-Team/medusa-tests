from constants import MED_OK, MED_NO
from framework import Register
from bitmap import Bitmap
import random

register = Register()

def printk(*args):
    s = Printk()
    for m in args:
        msg = str(m)
        for i in msg.split('\n'):
            s.message = i
            s.update()

@register('init')
def init():
    tmp = Fuck()
    tmp.action = 'hocico'
    tmp.path = '/home/jano/asd.txt'
    tmp.fetch()
    printk(tmp)
    #print(tmp)
    #tmp.attr['action'].val = 'append'
    #tmp.attr['path'].val = '/home/jano/asd2.txt'
    #tmp.update()

    for i in range(1, 7):
        printk("cislo %d zije" % i)

@register('getprocess')
def getprocess(event, parent):
    if parent.gid == 0:
        printk("getprocess: parent gid ROOT")
    else:
        printk("getprocess: change parent gid %d of '%s' to ROOT" % (parent.gid, parent.cmdline))
        parent.gid = 0
    parent.med_oact = Bitmap(16)
    parent.med_oact.fill()
    parent.med_sact = Bitmap(16)
    parent.med_sact.fill()
    parent.update()
    print(parent)

    tmp = Process()
    print(tmp)

    #print(event)
    #print(parent)
    return MED_OK

@register('getfile', event={'filename': '/'})
def getfile(event, new_file, parent):
    #print('som root vyfiltrovany cez dictionary')
    #print(event)
    return MED_OK

@register('getfile', event={'filename': lambda x: x == '/'})
def getfile(event, new_file, parent):
    #print('som root vyfiltrovany cez lambdu v dictionary')
    #print(event)
    return MED_OK

@register('getfile', event=lambda e: e.filename == '/')
def getfile(event, new_file, parent):
    #print('som root vyfiltrovany cez lambdu')
    #print(event)
    return MED_OK

@register('getfile')
def getfile(event, new_file, parent):
    #print(event)
    printk("getfile('%s')" % event.filename)
    new_file.med_oact = Bitmap(b'\xff\xff')
    new_file.update()
    print(event.filename)
    print(new_file)
    return MED_OK

@register('kill')
def kill(event, subj, obj):
    return MED_NO

@register('fork')
def fork(event, subj):
    #if random.random() < 0.2:
    #    return MED_NO
    return MED_OK

'''
IPC hooks

ipc_object has this attributes:
  ipc_class - class of IPC object, can be:
      MED_IPC_SEM = 0 (for semaphores)
      MED_IPC_MSG = 1 (for messages)
      MED_IPC_SHM = 2 (for shared memory)
      MED_IPC_UNDEFINED = 3 (for IPC event with None IPC object)
  deleted - boolean value {0,1}, set if IPC object is deleted
  id - ???
  key - ???
  uid - owner's uid
  gid - owner's gid
  cuid - c-groups uid
  cgid - c-groups gid
  mode - ???
  seq - ???
  refcount - number of references to this IPC object
'''

MED_IPC_SEM = 0
MED_IPC_MSG = 1
MED_IPC_SHM = 2
MED_IPC_UNDEFINED = 3

@register('ipc_ctl')
def ipc_ctl(event, process, ipc_object):
    '''
    event.ipc_class - type of IPC object
    event.cmd - operation to be performed

    ipc_object can be None, e.g. for IPC_INFO or MSG_INFO
    or SEM_INFO or SHM_INFO event.cmd value
    '''
    str = "ipc_ctl: cmd = {}".format(event.cmd)
    if ipc_object:
        str += ", ipc_class = {}\n".format(event.ipc_class)
        str += "    ipc_object.id = {}\n".format(ipc_object.id)
        str += "    ipc_object.key = 0x{:08x}\n".format(ipc_object.key)
        str += "    ipc_object.uid = {}\n".format(ipc_object.uid)
        str += "    ipc_object.mode = 0{:03o}\n".format(ipc_object.mode)
        str += "    ipc_object.refcount = {}".format(ipc_object.refcount)
    printk(str)
    return MED_OK

@register('ipc_associate')
def ipc_associate(event, process, ipc_object):
    '''
    event.ipc_class - type of IPC object
    event.flag - operation control flags
    '''
    str = "ipc_associate: ipc_class = {}, flag = 0x{:08x}".format(event.ipc_class, event.flag)
    printk(str)

    ipc_object.uid = 0
    ipc_object.gid = 1234
    ipc_object.mode = 0o777
    ipc_object.update()
    return MED_OK

@register('ipc_perm')
def ipc_perm(event, process, ipc_object):
    '''
    event.ipc_class - tpe of IPC object
    event.perms - desired (requested) permission set
    '''
    str = "ipc_perm: ipc_class = {}, perms = 0{:03o}".format(event.ipc_class, event.perms)
    printk(str)
    return MED_OK

@register('ipc_semop')
def ipc_semop(event, process, ipc_object):
    '''
    event.ipc_class - type of IPC object, in this case semaphore array
    event.sem_num - semaphore index in array
    event.sem_op - operation to perform
    event.sem_flg - operation flags
    event.nsops - number of operations to perform
    event.alter - if set, indicates whether changes on sem array are to be made
    '''
    str = "ipc_semop: ipc_class = {}, sem_num = {}, sem_op = {}, sem_flg = 0x{:08x}, nsops = {}, alter = {}".format(event.ipc_class, event.sem_num, event.sem_op, event.sem_flg, event.nsops, event.alter)
    printk(str)
    return MED_OK

@register('ipc_shmat')
def ipc_shmat(event, process, ipc_object):
    '''
    event.ipc_class - type of IPC object, in this case shared memory
    event.shmaddr - address to attach memory region to
    event.shmflg - operational flags
    '''
    str = "ipc_shmat: ipc_class = {}, shmaddr = 0x{:08x}, shmflg = 0x{:08x}".format(event.ipc_class, event.shmaddr, event.shmflg)
    printk(str)
    return MED_OK

@register('ipc_msgrcv')
def ipc_msgrcv(event, process, ipc_object):
    '''
    event.ipc_class - type of IPC object (in this case message queue)
    event.m_type - message type
    event.m_ts - message text size
    event.mode - operational flags
    event.type - type of requested message
    event.target - pid of task structure for recipient process
    '''
    str = "ipc_msgrcv: ipc_class = {}, m_type = {}, m_ts = {}, mode = 0x{:08x}, type = {}, target = {}".format(event.ipc_class, event.m_type, event.m_ts, event.mode, event.type, event.target)
    printk(str)
    return MED_OK

@register('ipc_msgsnd')
def ipc_msgsnd(event, process, ipc_object):
    '''
    event.ipc_class - type of IPC object (in this case message queue)
    event.m_type - message type to sent
    event.m_ts - message text size
    event.msgflg - operational flags
    '''
    str = "ipc_msgsnd: ipc_class = {}, m_type = {}, m_ts = {}, msgflg = 0x{:08x}".format(event.ipc_class, event.m_type, event.m_ts, event.msgflg)
    printk(str)
    return MED_OK

@register('getipc')
def ipc(event, ipc_object):
    '''
    event.ipc_class - type of IPC object @ipc_object to be validated
    '''
    str = "getipc: ipc_class = {}\n".format(event.ipc_class)
    str += "    ipc_object.id = {}\n".format(ipc_object.id)
    str += "    ipc_object.key = 0x{:08x}\n".format(ipc_object.key)
    str += "    ipc_object.uid = {}\n".format(ipc_object.uid)
    str += "    ipc_object.mode = 0{:03o}\n".format(ipc_object.mode)
    str += "    ipc_object.refcount = {}".format(ipc_object.refcount)
    printk(str)
    return MED_OK

