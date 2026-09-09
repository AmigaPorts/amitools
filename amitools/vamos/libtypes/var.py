from amitools.vamos.libstructs import LocalVarStruct
from amitools.vamos.astructs import AmigaClassDef


@AmigaClassDef
class LocalVar(LocalVarStruct):
    """a struct LocalVar: a shell variable on a process's pr_LocalVars list.
    The name is stored right behind the struct, the value in its own block."""

    @classmethod
    def alloc_var(cls, alloc, name, vtype, flags=0):
        var = cls.alloc(
            alloc, tag="LocalVar(%s)" % name, size=cls.get_size() + len(name) + 1
        )
        name_addr = var.addr + cls.get_size()
        var._mem.w_cstr(name_addr, name)
        var.node.name.aptr = name_addr
        var.node.type.val = vtype
        var.node.pri.val = 0
        var.flags.val = flags
        var.value.aptr = 0
        var.len.val = 0
        return var

    @property
    def name(self):
        return self.node.name.str

    def set_value(self, alloc, size, src_addr=None, value=None):
        """store the block at src_addr, or the string value"""
        self.free_value(alloc)
        buf_addr = alloc.alloc_memory(size, label="LocalVarValue").addr
        self.value.aptr = buf_addr
        self.len.val = size
        if src_addr is not None:
            self._mem.copy_block(src_addr, buf_addr, size)
        else:
            self._mem.w_cstr(buf_addr, value)

    def free_value(self, alloc):
        if self.value.aptr != 0:
            alloc.free_memory(alloc.get_memory(self.value.aptr))
            self.value.aptr = 0
            self.len.val = 0

    def free_var(self, alloc):
        """unlink from its list and free value and node"""
        self.free_value(alloc)
        self.node.remove()
        self.free(alloc)
