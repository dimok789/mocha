# may or may not be inspired by plutoo's ctrrpc
import errno    
import socket
import os
import sys
import struct
from time import sleep

def buffer(size):
    return bytearray([0x00] * size)

def copy_string(buffer, s, offset):
    s += "\0"
    buffer[offset : (offset + len(s))] = bytearray(s, "ascii")

def copy_word(buffer, w, offset):
    buffer[offset : (offset + 4)] = struct.pack(">I", w)

def get_string(buffer, offset):
    s = buffer[offset:]
    if b'\x00' in s:
        return s[:s.index(b'\x00')].decode("utf-8")
    else:
        return s.decode("utf-8")

class wupclient:
    s=None

    def __init__(self, ip='192.168.178.23', port=1337):
        self.s=socket.socket()
        self.s.connect((ip, port))
        self.fsa_handle = None
        self.cwd = "/vol/storage_mlc01"

    def __del__(self):
        if self.fsa_handle != None:
            self.close(self.fsa_handle)
            self.fsa_handle = None

    # fundamental comms
    def send(self, command, data):
        request = struct.pack('>I', command) + data

        self.s.send(request)
        response = self.s.recv(0x600)

        ret = struct.unpack(">I", response[:4])[0]
        return (ret, response[4:])

    # core commands
    def read(self, addr, len):
        data = struct.pack(">II", addr, len)
        ret, data = self.send(1, data)
        if ret == 0:
            return data
        else:
            print("read error : %08X" % ret)
            return None

    def write(self, addr, data):
        data = struct.pack(">I", addr) + data
        ret, data = self.send(0, data)
        if ret == 0:
            return ret
        else:
            print("write error : %08X" % ret)
            return None

    def svc(self, svc_id, arguments):
        data = struct.pack(">I", svc_id)
        for a in arguments:
            data += struct.pack(">I", a)
        ret, data = self.send(2, data)
        if ret == 0:
            return struct.unpack(">I", data)[0]
        else:
            print("svc error : %08X" % ret)
            return None

    def kill(self):
        ret, _ = self.send(3, bytearray())
        return ret

    def memcpy(self, dst, src, len):
        data = struct.pack(">III", dst, src, len)
        ret, data = self.send(4, data)
        if ret == 0:
            return ret
        else:
            print("memcpy error : %08X" % ret)
            return None

    def repeatwrite(self, dst, val, n):
        data = struct.pack(">III", dst, val, n)
        ret, data = self.send(5, data)
        if ret == 0:
            return ret
        else:
            print("repeatwrite error : %08X" % ret)
            return None

    # derivatives
    def alloc(self, size, align = None):
        if size == 0:
            return 0
        if align == None:
            return self.svc(0x27, [0xCAFF, size])
        else:
            return self.svc(0x28, [0xCAFF, size, align])

    def free(self, address):
        if address == 0:
            return 0
        return self.svc(0x29, [0xCAFF, address])

    def load_buffer(self, b, align = None):
        if len(b) == 0:
            return 0
        address = self.alloc(len(b), align)
        self.write(address, b)
        return address

    def load_string(self, s, align = None):
        return self.load_buffer(bytearray(s + "\0", "ascii"), align)

    def open(self, device, mode):
        address = self.load_string(device)
        handle = self.svc(0x33, [address, mode])
        self.free(address)
        return handle

    def close(self, handle):
        return self.svc(0x34, [handle])

    def ioctl(self, handle, cmd, inbuf, outbuf_size):
        in_address = self.load_buffer(inbuf)
        out_data = None
        if outbuf_size > 0:
            out_address = self.alloc(outbuf_size)
            ret = self.svc(0x38, [handle, cmd, in_address, len(inbuf), out_address, outbuf_size])
            out_data = self.read(out_address, outbuf_size)
            self.free(out_address)
        else:
            ret = self.svc(0x38, [handle, cmd, in_address, len(inbuf), 0, 0])
        self.free(in_address)
        return (ret, out_data)

    def iovec(self, vecs):
        data = bytearray()
        for (a, s) in vecs:
            data += struct.pack(">III", a, s, 0)
        return self.load_buffer(data)

    def ioctlv(self, handle, cmd, inbufs, outbuf_sizes, inbufs_ptr = [], outbufs_ptr = []):
        inbufs = [(self.load_buffer(b, 0x40), len(b)) for b in inbufs]
        outbufs = [(self.alloc(s, 0x40), s) for s in outbuf_sizes]
        iovecs = self.iovec(inbufs + inbufs_ptr + outbufs_ptr + outbufs)
        out_data = []
        ret = self.svc(0x39, [handle, cmd, len(inbufs + inbufs_ptr), len(outbufs + outbufs_ptr), iovecs])
        for (a, s) in outbufs:
            out_data += [self.read(a, s)]
        for (a, _) in (inbufs + outbufs):
            self.free(a)
        self.free(iovecs)
        return (ret, out_data)

    # fsa
    def FSA_Mount(self, handle, device_path, volume_path, flags):
        inbuffer = buffer(0x520)
        copy_string(inbuffer, device_path, 0x0004)
        copy_string(inbuffer, volume_path, 0x0284)
        copy_word(inbuffer, flags, 0x0504)
        (ret, _) = self.ioctlv(handle, 0x01, [inbuffer, bytearray()], [0x293])
        return ret

    def FSA_Unmount(self, handle, path, flags):
        inbuffer = buffer(0x520)
        copy_string(inbuffer, path, 0x4)
        copy_word(inbuffer, flags, 0x284)
        (ret, _) = self.ioctl(handle, 0x02, inbuffer, 0x293)
        return ret

    def FSA_RawOpen(self, handle, device):
        inbuffer = buffer(0x520)
        copy_string(inbuffer, device, 0x4)
        (ret, data) = self.ioctl(handle, 0x6A, inbuffer, 0x293)
        return (ret, struct.unpack(">I", data[4:8])[0])

    def FSA_OpenDir(self, handle, path):
        inbuffer = buffer(0x520)
        copy_string(inbuffer, path, 0x4)
        (ret, data) = self.ioctl(handle, 0x0A, inbuffer, 0x293)
        return (ret, struct.unpack(">I", data[4:8])[0])

    def FSA_ReadDir(self, handle, dir_handle):
        inbuffer = buffer(0x520)
        copy_word(inbuffer, dir_handle, 0x4)
        (ret, data) = self.ioctl(handle, 0x0B, inbuffer, 0x293)
        data = bytearray(data[4:])
        unk = data[:0x64]
        if ret == 0:
            return (ret, {"name" : get_string(data, 0x64), "is_file" : (unk[0] & 128) != 128, "unk" : unk})
        else:
            return (ret, None)

    def FSA_CloseDir(self, handle, dir_handle):
        inbuffer = buffer(0x520)
        copy_word(inbuffer, dir_handle, 0x4)
        (ret, data) = self.ioctl(handle, 0x0D, inbuffer, 0x293)
        return ret

    def FSA_OpenFile(self, handle, path, mode):
        inbuffer = buffer(0x520)
        copy_string(inbuffer, path, 0x4)
        copy_string(inbuffer, mode, 0x284)
        (ret, data) = self.ioctl(handle, 0x0E, inbuffer, 0x293)
        return (ret, struct.unpack(">I", data[4:8])[0])

    def FSA_MakeDir(self, handle, path, flags):
        inbuffer = buffer(0x520)
        copy_string(inbuffer, path, 0x4)
        copy_word(inbuffer, flags, 0x284)
        (ret, _) = self.ioctl(handle, 0x07, inbuffer, 0x293)
        return ret

    def FSA_ReadFile(self, handle, file_handle, size, cnt):
        inbuffer = buffer(0x520)
        copy_word(inbuffer, size, 0x08)
        copy_word(inbuffer, cnt, 0x0C)
        copy_word(inbuffer, file_handle, 0x14)
        (ret, data) = self.ioctlv(handle, 0x0F, [inbuffer], [size * cnt, 0x293])
        return (ret, data[0])

    def FSA_WriteFile(self, handle, file_handle, data):
        inbuffer = buffer(0x520)
        copy_word(inbuffer, 1, 0x08) # size
        copy_word(inbuffer, len(data), 0x0C) # cnt
        copy_word(inbuffer, file_handle, 0x14)
        (ret, data) = self.ioctlv(handle, 0x10, [inbuffer, data], [0x293])
        return (ret)

    def FSA_ReadFilePtr(self, handle, file_handle, size, cnt, ptr):
        inbuffer = buffer(0x520)
        copy_word(inbuffer, size, 0x08)
        copy_word(inbuffer, cnt, 0x0C)
        copy_word(inbuffer, file_handle, 0x14)
        (ret, data) = self.ioctlv(handle, 0x0F, [inbuffer], [0x293], [], [(ptr, size*cnt)])
        return (ret, data[0])

    def FSA_WriteFilePtr(self, handle, file_handle, size, cnt, ptr):
        inbuffer = buffer(0x520)
        copy_word(inbuffer, size, 0x08)
        copy_word(inbuffer, cnt, 0x0C)
        copy_word(inbuffer, file_handle, 0x14)
        (ret, data) = self.ioctlv(handle, 0x10, [inbuffer], [0x293], [(ptr, size*cnt)], [])
        return (ret)

    def FSA_GetStatFile(self, handle, file_handle):
        inbuffer = buffer(0x520)
        copy_word(inbuffer, file_handle, 0x4)
        (ret, data) = self.ioctl(handle, 0x14, inbuffer, 0x64)
        return (ret, struct.unpack(">IIIIIIIIIIIIIIIIIIIIIIIII", data))

    def FSA_CloseFile(self, handle, file_handle):
        inbuffer = buffer(0x520)
        copy_word(inbuffer, file_handle, 0x4)
        (ret, data) = self.ioctl(handle, 0x15, inbuffer, 0x293)
        return ret

    def FSA_ChangeMode(self, handle, path, mode):
        mask = 0x777
        inbuffer = buffer(0x520)
        copy_string(inbuffer, path, 0x0004)
        copy_word(inbuffer, mode, 0x0284)
        copy_word(inbuffer, mask, 0x0288)
        (ret, _) = self.ioctl(handle, 0x20, inbuffer, 0x293)
        return ret

    # mcp
    def MCP_InstallGetInfo(self, handle, path):
        inbuffer = buffer(0x27F)
        copy_string(inbuffer, path, 0x0)
        (ret, data) = self.ioctlv(handle, 0x80, [inbuffer], [0x16])
        return (ret, struct.unpack(">IIIIIH", data[0]))

    def MCP_Install(self, handle, path):
        inbuffer = buffer(0x27F)
        copy_string(inbuffer, path, 0x0)
        (ret, _) = self.ioctlv(handle, 0x81, [inbuffer], [])
        return ret

    def MCP_InstallGetProgress(self, handle):
        (ret, data) = self.ioctl(handle, 0x82, [], 0x24)
        return (ret, struct.unpack(">IIIIIIIII", data))

    def MCP_CopyTitle(self, handle, path, dst_device_id, flush):
        inbuffer = buffer(0x27F)
        copy_string(inbuffer, path, 0x0)
        inbuffer2 = buffer(0x4)
        copy_word(inbuffer2, dst_device_id, 0x0)
        inbuffer3 = buffer(0x4)
        copy_word(inbuffer3, flush, 0x0)
        (ret, _) = self.ioctlv(handle, 0x85, [inbuffer, inbuffer2, inbuffer3], [])
        return ret

    def MCP_InstallSetTargetDevice(self, handle, device):
        inbuffer = buffer(0x4)
        copy_word(inbuffer, device, 0x0)
        (ret, _) = self.ioctl(handle, 0x8D, inbuffer, 0)
        return ret

    def MCP_InstallSetTargetUsb(self, handle, device):
        inbuffer = buffer(0x4)
        copy_word(inbuffer, device, 0x0)
        (ret, _) = self.ioctl(handle, 0xF1, inbuffer, 0)
        return ret

    # syslog (tmp)
    def dump_syslog(self):
        syslog_address = struct.unpack(">I", self.read(0x05095ECC, 4))[0] + 0x10
        block_size = 0x400
        for i in range(0, 0x40000, block_size):
            data = self.read(syslog_address + i, 0x400)
            # if 0 in data:
            #     print(data[:data.index(0)].decode("ascii"))
            #     break
            # else:
            print(data)

    # file management
    def get_fsa_handle(self):
        if self.fsa_handle == None:
            self.fsa_handle = self.open("/dev/fsa", 0)
        return self.fsa_handle

    def mkdir(self, path, flags):
        fsa_handle = self.get_fsa_handle()
        if path[0] != "/":
            path = self.cwd + "/" + path
        ret = w.FSA_MakeDir(fsa_handle, path, flags)
        if ret == 0:
            return 0
        else:
            print("mkdir error (%s, %08X)" % (path, ret))
            return ret

    def chmod(self, filename, flags):
        fsa_handle = self.get_fsa_handle()
        if filename[0] != "/":
            filename = self.cwd + "/" + filename
        ret = w.FSA_ChangeMode(fsa_handle, filename, flags)
        print("chmod returned : " + hex(ret))
        
    def cd(self, path):
        if path[0] != "/" and self.cwd[0] == "/":
            return self.cd(self.cwd + "/" + path)
        fsa_handle = self.get_fsa_handle()
        ret, dir_handle = self.FSA_OpenDir(fsa_handle, path if path != None else self.cwd)
        if ret == 0:
            self.cwd = path
            self.FSA_CloseDir(fsa_handle, dir_handle)
            return 0
        else:
            print("cd error : path does not exist (%s)" % (path))
            return -1

    def ls(self, path = None, return_data = False):
        fsa_handle = self.get_fsa_handle()
        if path != None and path[0] != "/":
            path = self.cwd + "/" + path
        ret, dir_handle = self.FSA_OpenDir(fsa_handle, path if path != None else self.cwd)
        if ret != 0x0:
            print("opendir error : " + hex(ret))
            return [] if return_data else None
        entries = []
        while True:
            ret, data = self.FSA_ReadDir(fsa_handle, dir_handle)
            if ret != 0:
                break
            if not(return_data):
                if data["is_file"]:
                    print("     %s" % data["name"])
                else:
                    print("     %s/" % data["name"])
            else:
                entries += [data]
        ret = self.FSA_CloseDir(fsa_handle, dir_handle)
        return entries if return_data else None

    def dldir(self, path):
        if path[0] != "/":
            path = self.cwd + "/" + path
        entries = self.ls(path, True)
        for e in entries:
            if e["is_file"]:
                print(e["name"])
                self.dl(path + "/" + e["name"],path[1:])
            else:
                print(e["name"] + "/")
                self.dldir(path + "/" + e["name"])
    
    def cpdir(self, srcpath, dstpath):
        entries = self.ls(srcpath, True)
        q = [(srcpath, dstpath, e) for e in entries]
        while len(q) > 0:
            _srcpath, _dstpath, e = q.pop()
            _srcpath += "/" + e["name"]
            _dstpath += "/" + e["name"]
            if e["is_file"]:
                print(e["name"])
                self.cp(_srcpath, _dstpath)
            else:
                self.mkdir(_dstpath, 0x600)
                entries = self.ls(_srcpath, True)
                q += [(_srcpath, _dstpath, e) for e in entries]

    def pwd(self):
        return self.cwd

    def cp(self, filename_in, filename_out):
        fsa_handle = self.get_fsa_handle()
        ret, in_file_handle = self.FSA_OpenFile(fsa_handle, filename_in, "r")
        if ret != 0x0:
            print("cp error : could not open " + filename_in)
            return
        ret, out_file_handle = self.FSA_OpenFile(fsa_handle, filename_out, "w")
        if ret != 0x0:
            print("cp error : could not open " + filename_out)
            return
        block_size = 0x10000
        buffer = self.alloc(block_size, 0x40)
        k = 0
        while True:
            ret, _ = self.FSA_ReadFilePtr(fsa_handle, in_file_handle, 0x1, block_size, buffer)
            k += ret
            ret = self.FSA_WriteFilePtr(fsa_handle, out_file_handle, 0x1, ret, buffer)
            sys.stdout.write(hex(k) + "\r"); sys.stdout.flush();
            if ret < block_size:
                break
        self.free(buffer)
        ret = self.FSA_CloseFile(fsa_handle, out_file_handle)
        ret = self.FSA_CloseFile(fsa_handle, in_file_handle)

    def df(self, filename_out, src, size):
        fsa_handle = self.get_fsa_handle()
        ret, out_file_handle = self.FSA_OpenFile(fsa_handle, filename_out, "w")
        if ret != 0x0:
            print("df error : could not open " + filename_out)
            return
        block_size = 0x10000
        buffer = self.alloc(block_size, 0x40)
        k = 0
        while k < size:
            cur_size = min(size - k, block_size)
            self.memcpy(buffer, src + k, cur_size)
            k += cur_size
            ret = self.FSA_WriteFilePtr(fsa_handle, out_file_handle, 0x1, cur_size, buffer)
            sys.stdout.write(hex(k) + " (%f) " % (float(k * 100) / size) + "\r"); sys.stdout.flush();
        self.free(buffer)
        ret = self.FSA_CloseFile(fsa_handle, out_file_handle)

    def dl(self, filename, directorypath = None, local_filename = None):
        fsa_handle = self.get_fsa_handle()
        if filename[0] != "/":
            filename = self.cwd + "/" + filename
        if local_filename == None:
            if "/" in filename:
                local_filename = filename[[i for i, x in enumerate(filename) if x == "/"][-1]+1:]
            else:
                local_filename = filename
        ret, file_handle = self.FSA_OpenFile(fsa_handle, filename, "r")
        if ret != 0x0:
            print("dl error : could not open " + filename)
            return
        buffer = bytearray()
        block_size = 0x400
        while True:
            ret, data = self.FSA_ReadFile(fsa_handle, file_handle, 0x1, block_size)
            # print(hex(ret), data)
            buffer += data[:ret]
            sys.stdout.write(hex(len(buffer)) + "\r"); sys.stdout.flush();
            if ret < block_size:
                break
        ret = self.FSA_CloseFile(fsa_handle, file_handle)
        if directorypath == None:
            open(local_filename, "wb").write(buffer)
        else:
            dir_path = os.path.dirname(os.path.abspath(sys.argv[0])).replace('\\','/')
            fullpath = dir_path + "/" + directorypath + "/"
            fullpath = fullpath.replace("//","/")
            mkdir_p(fullpath)
            open(fullpath + local_filename, "wb").write(buffer) 

    def mkdir_p(path):
        try:
            os.makedirs(path)
        except OSError as exc:  # Python >2.5
            if exc.errno == errno.EEXIST and os.path.isdir(path):
                pass
            else:
                raise
            
    def fr(self, filename, offset, size):
        fsa_handle = self.get_fsa_handle()
        if filename[0] != "/":
            filename = self.cwd + "/" + filename
        ret, file_handle = self.FSA_OpenFile(fsa_handle, filename, "r")
        if ret != 0x0:
            print("fr error : could not open " + filename)
            return
        buffer = bytearray()
        block_size = 0x400
        while True:
            ret, data = self.FSA_ReadFile(fsa_handle, file_handle, 0x1, block_size if (block_size < size) else size)
            buffer += data[:ret]
            sys.stdout.write(hex(len(buffer)) + "\r"); sys.stdout.flush();
            if len(buffer) >= size:
                break
        ret = self.FSA_CloseFile(fsa_handle, file_handle)
        return buffer

    def fw(self, filename, offset, buffer):
        fsa_handle = self.get_fsa_handle()
        if filename[0] != "/":
            filename = self.cwd + "/" + filename
        ret, file_handle = self.FSA_OpenFile(fsa_handle, filename, "r+")
        if ret != 0x0:
            print("fw error : could not open " + filename)
            return
        block_size = 0x400
        k = 0
        while True:
            cur_size = min(len(buffer) - k, block_size)
            if cur_size <= 0:
                break
            sys.stdout.write(hex(k) + "\r"); sys.stdout.flush();
            ret = self.FSA_WriteFile(fsa_handle, file_handle, buffer[k:(k+cur_size)])
            k += cur_size
        ret = self.FSA_CloseFile(fsa_handle, file_handle)

    def stat(self, filename):
        fsa_handle = self.get_fsa_handle()
        if filename[0] != "/":
            filename = self.cwd + "/" + filename
        ret, file_handle = self.FSA_OpenFile(fsa_handle, filename, "r")
        if ret != 0x0:
            print("stat error : could not open " + filename)
            return
        (ret, stats) = self.FSA_GetStatFile(fsa_handle, file_handle)
        if ret != 0x0:
            print("stat error : " + hex(ret))
        else:
            print("flags: " + hex(stats[1]))
            print("mode: " + hex(stats[2]))
            print("owner: " + hex(stats[3]))
            print("group: " + hex(stats[4]))
            print("size: " + hex(stats[5]))
        ret = self.FSA_CloseFile(fsa_handle, file_handle)

    def up(self, local_filename, filename = None):
        fsa_handle = self.get_fsa_handle()
        if filename == None:
            if "/" in local_filename:
                filename = local_filename[[i for i, x in enumerate(local_filename) if x == "/"][-1]+1:]
            else:
                filename = local_filename
        if filename[0] != "/":
            filename = self.cwd + "/" + filename
        f = open(local_filename, "rb")
        ret, file_handle = self.FSA_OpenFile(fsa_handle, filename, "w")
        if ret != 0x0:
            print("up error : could not open " + filename)
            return
        progress = 0
        block_size = 0x400
        while True:
            data = f.read(block_size)
            ret = self.FSA_WriteFile(fsa_handle, file_handle, data)
            progress += len(data)
            sys.stdout.write(hex(progress) + "\r"); sys.stdout.flush();
            if len(data) < block_size:
                break
        ret = self.FSA_CloseFile(fsa_handle, file_handle)

def mkdir_p(path):
    try:
        os.makedirs(path)
    except OSError as exc:
        if exc.errno == errno.EEXIST and os.path.isdir(path):
            pass
        else:
            raise

def mount_sd():
    handle = w.open("/dev/fsa", 0)
    print(hex(handle))

    ret = w.FSA_Mount(handle, "/dev/sdcard01", "/vol/storage_sdcard", 2)
    print(hex(ret))

    ret = w.close(handle)
    print(hex(ret))

def unmount_sd():
    handle = w.open("/dev/fsa", 0)
    print(hex(handle))

    ret = w.FSA_Unmount(handle, "/vol/storage_sdcard", 2)
    print(hex(ret))

    ret = w.close(handle)
    print(hex(ret))

def mount_slccmpt01():
    handle = w.open("/dev/fsa", 0)
    print(hex(handle))

    ret = w.FSA_Mount(handle, "/dev/slccmpt01", "/vol/storage_slccmpt01", 2)
    print(hex(ret))

    ret = w.close(handle)
    print(hex(ret))

def unmount_slccmpt01():
    handle = w.open("/dev/fsa", 0)
    print(hex(handle))

    ret = w.FSA_Unmount(handle, "/vol/storage_slccmpt01", 2)
    print(hex(ret))

    ret = w.close(handle)
    print(hex(ret))

def mount_odd_content():
    handle = w.open("/dev/fsa", 0)
    print(hex(handle))

    ret = w.FSA_Mount(handle, "/dev/odd03", "/vol/storage_odd_content", 2)
    print(hex(ret))

    ret = w.close(handle)
    print(hex(ret))

def unmount_odd_content():
    handle = w.open("/dev/fsa", 0)
    print(hex(handle))

    ret = w.FSA_Unmount(handle, "/vol/storage_odd_content", 2)
    print(hex(ret))

    ret = w.close(handle)
    print(hex(ret))

def mount_odd_update():
    handle = w.open("/dev/fsa", 0)
    print(hex(handle))

    ret = w.FSA_Mount(handle, "/dev/odd02", "/vol/storage_odd_update", 2)
    print(hex(ret))

    ret = w.close(handle)
    print(hex(ret))

def unmount_odd_update():
    handle = w.open("/dev/fsa", 0)
    print(hex(handle))

    ret = w.FSA_Unmount(handle, "/vol/storage_odd_update", 2)
    print(hex(ret))

    ret = w.close(handle)
    print(hex(ret))

def mount_odd_tickets():
    handle = w.open("/dev/fsa", 0)
    print(hex(handle))

    ret = w.FSA_Mount(handle, "/dev/odd01", "/vol/storage_odd_tickets", 2)
    print(hex(ret))

    ret = w.close(handle)
    print(hex(ret))

def unmount_odd_tickets():
    handle = w.open("/dev/fsa", 0)
    print(hex(handle))

    ret = w.FSA_Unmount(handle, "/vol/storage_odd_tickets", 2)
    print(hex(ret))

    ret = w.close(handle)
    print(hex(ret))

def install_title(path, installToUsb = 0):
    mcp_handle = w.open("/dev/mcp", 0)
    print(hex(mcp_handle))

    ret, data = w.MCP_InstallGetInfo(mcp_handle, "/vol/storage_sdcard/"+path)
    print("install info : " + hex(ret), [hex(v) for v in data])
    if ret != 0:
        ret = w.close(mcp_handle)
        print(hex(ret))
        return

    ret = w.MCP_InstallSetTargetDevice(mcp_handle, installToUsb)
    print("install set target device : " + hex(ret))
    if ret != 0:
        ret = w.close(mcp_handle)
        print(hex(ret))
        return

    ret = w.MCP_InstallSetTargetUsb(mcp_handle, installToUsb)
    print("install set target usb : " + hex(ret))
    if ret != 0:
        ret = w.close(mcp_handle)
        print(hex(ret))
        return

    ret = w.MCP_Install(mcp_handle, "/vol/storage_sdcard/"+path)
    print("install : " + hex(ret))

    ret = w.close(mcp_handle)
    print(hex(ret))

def get_nim_status():
    nim_handle = w.open("/dev/nim", 0)
    print(hex(nim_handle))
    
    inbuffer = buffer(0x80)
    (ret, data) = w.ioctlv(nim_handle, 0x00, [inbuffer], [0x80])

    print(hex(ret), "".join("%02X" % v for v in data[0]))

    ret = w.close(nim_handle)
    print(hex(ret))

def read_and_print(adr, size):
    data = w.read(adr, size)
    data = struct.unpack(">%dI" % (len(data) // 4), data)
    for i in range(0, len(data), 4):
        print(" ".join("%08X"%v for v in data[i:i+4]))

if __name__ == '__main__':
    w = wupclient()
    # mount_sd()
    # mount_odd_content()

    # w.up("/media/harddisk1/loadiine_code/homebrew_launcher_rpx/homebrew_launcher.rpx", "homebrew_launcher_rpx.rpx")
    # w.up("/media/harddisk1/gx2sploit/gx2sploit.rpx", "homebrew_launcher_rpx.rpx")
    # print(w.pwd())
    # w.ls()
    w.dump_syslog()
    # w.mkdir("/vol/storage_sdcard/usr", 0x600)
    # install_title("install", 1)
    # get_nim_status()
    # w.kill()
