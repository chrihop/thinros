from ctypes import *
import os


class TopicPartition(Structure):
    _fields_ = [
        ('partition_id', c_size_t),
        ('status', c_size_t),
    ]


class ThinROSIPC(Structure):
    _fields_ = [
        ("fd", c_int),
        ("par", c_void_p)]


class ThinROS:
    def __init__(self):
        lib_path = os.path.join(
            os.path.dirname(os.path.realpath(__file__)),
            'libthinros.so')
        self.lib = cdll.LoadLibrary(lib_path)
        ipc = self.lib.thinros_bind()
        self.ipc = ThinROSIPC.from_address(ipc)
        self.par = TopicPartition.from_address(self.ipc.par)

    def __del__(self):
        self.lib.thinros_unbind(pointer(self.ipc))

    def topic_partition_init(self):
        if self.par.status != 0:
            return
        self.lib.topic_partition_init(pointer(self.par))

    def topic_nonsecure_partition_init(self):
        self.lib.topic_nonsecure_partition_init(pointer(self.par))

    def thinros_node(self):
        pass


class Test:
    @staticmethod
    def main():
        thinros = ThinROS()
        print(thinros.ipc.fd, thinros.ipc.par)


if __name__ == '__main__':
    Test.main()


