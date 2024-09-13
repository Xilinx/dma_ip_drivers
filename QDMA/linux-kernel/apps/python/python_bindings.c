#include <Python.h>
#include <structmember.h>

#include <stddef.h>

#include <dmautils.h>
#include <qdma_nl.h>

#define QDMA_MODE_ST  (1 << 0)
#define QDMA_MODE_MM  (1 << 1)
#define QDMA_DIR_H2C  (1 << 0)
#define QDMA_DIR_C2H  (1 << 1)

static void xnl_dump_response(const char *resp)
{
    printf("%s", resp);
}



typedef struct {
    PyObject_HEAD
    /* Type-specific fields go here. */
    unsigned int if_bdf;
} APIObject;


static PyObject* qdma_new(PyTypeObject* type, PyObject* args, PyObject* kwds) {
    APIObject* self;

    self = (APIObject*)type->tp_alloc(type, 0);
    self->if_bdf = 0;

    return (PyObject *)self;
}


static void qdma_dealloc(APIObject* self)
{
    Py_TYPE(self)->tp_free((PyObject*)self);
}

static int qdma_init(APIObject* self, PyObject* args, PyObject* kwds) {
    static char *kwlist[] = {"if_bdf", NULL};

    if (!PyArg_ParseTupleAndKeywords(args, kwds, "I", kwlist, &self->if_bdf)) {
        return -1;
    }

    return 0;
}

static PyObject *q_cmd (unsigned int cmd, unsigned int if_bdf, unsigned int idx, unsigned int mode,
        unsigned int dir, int (*xnl_proc_fn)(struct xcmd_info *xcmd))
{
    struct xcmd_info xcmd;
    int rv;

    memset(&xcmd, 0, sizeof(struct xcmd_info));
    xcmd.log_msg_dump = xnl_dump_response;
    xcmd.if_bdf = if_bdf;
    xcmd.vf = 0;
    xcmd.op = cmd;
    xcmd.req.qparm.idx = idx;
    xcmd.req.qparm.num_q = 1;
    xcmd.req.qparm.sflags = 1 << QPARM_DIR | 1 << QPARM_MODE | 1 << QPARM_IDX;

    if (mode & QDMA_MODE_ST) {
        xcmd.req.qparm.flags |= XNL_F_QMODE_ST;
    } else if (mode & QDMA_MODE_MM) {
        xcmd.req.qparm.flags |= XNL_F_QMODE_MM;
    } else {
        PyErr_SetString(PyExc_ValueError, "mode must be either ST or MM");
        return NULL;
    }

    if ((dir & (QDMA_DIR_H2C | QDMA_DIR_C2H)) == 0) {
        PyErr_SetString(PyExc_ValueError, "dir must set H2C, C2H, or both");
        return NULL;
    }
    if (dir & QDMA_DIR_H2C) {
        xcmd.req.qparm.flags |= XNL_F_QDIR_H2C;
    }
    if (dir & QDMA_DIR_C2H) {
        xcmd.req.qparm.flags |= XNL_F_QDIR_C2H;
    }

    rv = (xnl_proc_fn)(&xcmd);
    if (rv) {
        PyErr_SetString(PyExc_RuntimeError, "Error sending add Q command");
        return NULL;
    }

    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject *q_add (APIObject *self, PyObject *args)
{
    unsigned int idx, mode, dir;

    if (!PyArg_ParseTuple (args, "III", &idx, &mode, &dir))
    {
        return NULL;
    }

    return q_cmd(XNL_CMD_Q_ADD, self->if_bdf, idx, mode, dir, qdma_q_add);
}

static PyObject *q_del (APIObject *self, PyObject *args)
{
    unsigned int idx, mode, dir;

    if (!PyArg_ParseTuple (args, "III", &idx, &mode, &dir))
    {
        return NULL;
    }

    return q_cmd(XNL_CMD_Q_DEL, self->if_bdf, idx, mode, dir, qdma_q_del);
}

static PyObject *q_start (APIObject *self, PyObject *args)
{
    unsigned int idx, mode, dir;
    struct xcmd_info xcmd;
    int rv;

    if (!PyArg_ParseTuple (args, "III", &idx, &mode, &dir))
    {
        return NULL;
    }

    memset(&xcmd, 0, sizeof(struct xcmd_info));
    xcmd.log_msg_dump = xnl_dump_response;
    xcmd.if_bdf = self->if_bdf;
    xcmd.vf = 0;
    xcmd.op = XNL_CMD_Q_START;
    xcmd.req.qparm.idx = idx;
    xcmd.req.qparm.num_q = 1;
    xcmd.req.qparm.sflags = 1 << QPARM_DIR | 1 << QPARM_MODE | 1 << QPARM_IDX | 1 << QPARM_RNGSZ_IDX;
    xcmd.req.qparm.fetch_credit = Q_ENABLE_C2H_FETCH_CREDIT;
    xcmd.req.qparm.qrngsz_idx = 9;
    xcmd.req.qparm.flags = (XNL_F_CMPL_STATUS_EN | XNL_F_CMPL_STATUS_ACC_EN |
            XNL_F_CMPL_STATUS_PEND_CHK | XNL_F_CMPL_STATUS_DESC_EN |
            XNL_F_FETCH_CREDIT);


    if (mode & QDMA_MODE_ST) {
        xcmd.req.qparm.flags |= XNL_F_QMODE_ST;
    } else if (mode & QDMA_MODE_MM) {
        xcmd.req.qparm.flags |= XNL_F_QMODE_MM;
    } else {
        PyErr_SetString(PyExc_ValueError, "mode must be either ST or MM");
        return NULL;
    }

    if ((dir & (QDMA_DIR_H2C | QDMA_DIR_C2H)) == 0) {
        PyErr_SetString(PyExc_ValueError, "dir must set H2C, C2H, or both");
        return NULL;
    }
    if (dir & QDMA_DIR_H2C) {
        xcmd.req.qparm.flags |= XNL_F_QDIR_H2C;
    }
    if (dir & QDMA_DIR_C2H) {
        xcmd.req.qparm.flags |= XNL_F_QDIR_C2H;
    }

    rv = qdma_q_start(&xcmd);
    if (rv) {
        PyErr_SetString(PyExc_RuntimeError, "Error sending add Q command");
        return NULL;
    }

    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject *q_stop (APIObject *self, PyObject *args)
{
    unsigned int idx, mode, dir;

    if (!PyArg_ParseTuple (args, "III", &idx, &mode, &dir))
    {
        return NULL;
    }

    return q_cmd(XNL_CMD_Q_STOP, self->if_bdf, idx, mode, dir, qdma_q_stop);
}


static PyMemberDef api_members[] = {
        {"if_bdf", T_INT, offsetof(APIObject, if_bdf), 0, "Queue BDF" },
        {NULL}
};

static PyMethodDef api_methods[] =
{
    {"q_add", (PyCFunction)q_add, METH_VARARGS, "q_idx, mode, dir"},
    {"q_del", (PyCFunction)q_del, METH_VARARGS, "q_idx, mode, dir"},
    {"q_start", (PyCFunction)q_start, METH_VARARGS, "q_idx, mode, dir"},
    {"q_stop", (PyCFunction)q_stop, METH_VARARGS, "q_idx, mode, dir"},

    {NULL, NULL, 0, NULL}        /* Sentinel */
};

static PyTypeObject qdma_APIType = {
    PyVarObject_HEAD_INIT(NULL, 0)
    "qdma.API",                /* tp_name */
    sizeof(APIObject),         /* tp_basicsize */
    0,                         /* tp_itemsize */
    (destructor)qdma_dealloc,  /* tp_dealloc */
    0,                         /* tp_print */
    0,                         /* tp_getattr */
    0,                         /* tp_setattr */
    0,                         /* tp_compare */
    0,                         /* tp_repr */
    0,                         /* tp_as_number */
    0,                         /* tp_as_sequence */
    0,                         /* tp_as_mapping */
    0,                         /* tp_hash */
    0,                         /* tp_call */
    0,                         /* tp_str */
    0,                         /* tp_getattro */
    0,                         /* tp_setattro */
    0,                         /* tp_as_buffer */
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE, /* tp_flags */
    "QDMA C API",              /* tp_doc */
    0,                         /* tp_traverse */
    0,                         /* tp_clear */
    0,                         /* tp_richcompare */
    0,                         /* tp_weaklistoffset */
    0,                         /* tp_iter */
    0,                         /* tp_iternext */
    api_methods,               /* tp_methods */
    api_members,               /* tp_members */
    0,                         /* tp_getset */
    0,                         /* tp_base */
    0,                         /* tp_dict */
    0,                         /* tp_descr_get */
    0,                         /* tp_descr_set */
    0,                         /* tp_dictoffset */
    (initproc)qdma_init,       /* tp_init */
    0,                         /* tp_alloc */
    PyType_GenericNew,         /* tp_new */
};

// copied from dma-utils/dma-xfer
#define QDMA_RW_MAX_SIZE    0x7ffff000

static ssize_t write_from_buffer(const char *fname, int fd, char *buffer,
        uint64_t size, uint64_t base)
{
    ssize_t rc;
    uint64_t count = 0;
    char *buf = buffer;
    off_t offset = base;

    do { /* Support zero byte transfer */
        uint64_t bytes = size - count;

        if (bytes > QDMA_RW_MAX_SIZE)
            bytes = QDMA_RW_MAX_SIZE;

        if (offset) {
            rc = lseek(fd, offset, SEEK_SET);
            if (rc < 0) {
                printf("Error: %s, seek off 0x%lx failed %zd\n",
                        fname, offset, rc);
                return -EIO;
            }
            if (rc != offset) {
                printf("Error: %s, seek off 0x%lx != 0x%lx\n",
                        fname, rc, offset);
                return -EIO;
            }
        }

        /* write data to file from memory buffer */
        rc = write(fd, buf, bytes);
        if (rc < 0) {
            printf("Failed to Read %s, read off 0x%lx + 0x%lx failed %zd\n",
                    fname, offset, bytes, rc);
            return -EIO;
        }
        if (rc != bytes) {
            printf("Failed to read %lx bytes from file %s, curr read:%lx\n",
                    bytes, fname, rc);
            return -EIO;
        }

        count += bytes;
        buf += bytes;
        offset += bytes;

    } while (count < size);

    if (count != size) {
        printf("Failed to read %lx bytes from %s 0x%lx != 0x%lx\n",
                size, fname, count, size);
        return -EIO;
    }

    return count;
}

static ssize_t read_to_buffer(const char *fname, int fd, char *buffer,
        uint64_t size, uint64_t base)
{
    ssize_t rc;
    uint64_t count = 0;
    char *buf = buffer;
    off_t offset = base;

    do { /* Support zero byte transfer */
        uint64_t bytes = size - count;

        if (bytes > QDMA_RW_MAX_SIZE)
            bytes = QDMA_RW_MAX_SIZE;

        if (offset) {
            rc = lseek(fd, offset, SEEK_SET);
            if (rc < 0) {
                printf("Error: %s, seek off 0x%lx failed %zd\n",
                        fname, offset, rc);
                return -EIO;
            }
            if (rc != offset) {
                printf("Error: %s, seek off 0x%lx != 0x%lx\n",
                        fname, rc, offset);
                return -EIO;
            }
        }

        /* read data from file into memory buffer */
        rc = read(fd, buf, bytes);
        if (rc < 0) {
            printf("Failed to Read %s, read off 0x%lx + 0x%lx failed %zd\n",
                    fname, offset, bytes, rc);
            return -EIO;
        }
        if (rc != bytes) {
            printf("Failed to read %lx bytes from file %s, curr read:%lx\n",
                    bytes, fname, rc);
            return -EIO;
        }

        count += bytes;
        buf += bytes;
        offset += bytes;

    } while (count < size);

    if (count != size) {
        printf("Failed to read %lx bytes from %s 0x%lx != 0x%lx.\n",
                size, fname, count, size);
        return -EIO;
    }

    return count;
}


static PyObject *qdma_write_from_buffer (PyObject *module, PyObject *args)
{
    int rv;
    int fd;
    char* buffer;
    uint64_t size;
    uint64_t base;

    if (!PyArg_ParseTuple (args, "iKKK", &fd, &buffer, &size, &base))
    {
        return NULL;
    }

    rv = write_from_buffer(__func__, fd, buffer, size, base);

    if (rv < 0) {
        PyErr_SetString(PyExc_RuntimeError, "Error calling write_from_buffer");
        return NULL;
    }

    return Py_BuildValue("i", rv);
}

static PyObject *qdma_read_to_buffer (PyObject *module, PyObject *args)
{
    int rv;
    int fd;
    char* buffer;
    uint64_t size;
    uint64_t base;

    if (!PyArg_ParseTuple (args, "iKKK", &fd, &buffer, &size, &base))
    {
        return NULL;
    }

    rv = read_to_buffer(__func__, fd, buffer, size, base);

    if (rv < 0) {
        PyErr_SetString(PyExc_RuntimeError, "Error calling write_from_buffer");
        return NULL;
    }

    return Py_BuildValue("i", rv);
}


static PyMethodDef module_methods[] =
{
    {"write_from_buffer", (PyCFunction)qdma_write_from_buffer, METH_VARARGS, "fd, buffer, size, base"},
    {"read_to_buffer", (PyCFunction)qdma_read_to_buffer, METH_VARARGS, "fd, buffer, size, base"},

    {NULL, NULL, 0, NULL}        /* Sentinel */
};

#if PY_MAJOR_VERSION >= 3
static struct PyModuleDef QdmaApiModule = {
    PyModuleDef_HEAD_INIT,
    "qdma_api",         /* m_name */
    NULL,               /* m_doc */
    -1,                 /* m_size */
    module_methods,     /* m_methods */
    NULL,               /* m_reload */
    NULL,               /* m_traverse */
    NULL,               /* m_clear */
    NULL,               /* m_free */
};
#define INIT_RETURN(m)  return m
#define INIT_FNAME  PyInit_qdma_api
#else
#define INIT_RETURN(m)  return
#define INIT_FNAME  initqdma_api
#endif

PyMODINIT_FUNC INIT_FNAME(void)
{
    if (PyType_Ready(&qdma_APIType) < 0)
        INIT_RETURN(NULL);        

#if PY_MAJOR_VERSION >= 3
    PyObject *m = PyModule_Create(&QdmaApiModule);
#else

    PyObject *m = Py_InitModule3 ("qdma_api", module_methods, "QDMA C API Module");
#endif

    Py_INCREF(&qdma_APIType);
    PyModule_AddObject(m, "API", (PyObject *)&qdma_APIType);

    INIT_RETURN(m);
}

