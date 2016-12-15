using System;
using System.IO;
using System.Net;
using System.Net.Sockets;
using System.Runtime.InteropServices;
using System.Text;
using System.Threading;
using System.Collections.Generic;

namespace cafiine_server
{
    class Program
    {
        // Command bytes
        public const byte BYTE_NORMAL = 0xff;
        public const byte BYTE_SPECIAL = 0xfe;
        public const byte BYTE_OK = 0xfd;
        public const byte BYTE_PING = 0xfc;
        public const byte BYTE_LOG_STR = 0xfb;

        public const byte BYTE_OPEN_FILE = 0x00;
        public const byte BYTE_OPEN_FILE_ASYNC = 0x01;
        public const byte BYTE_OPEN_DIR = 0x02;
        public const byte BYTE_OPEN_DIR_ASYNC = 0x03;
        public const byte BYTE_CHANGE_DIR = 0x04;
        public const byte BYTE_CHANGE_DIR_ASYNC = 0x05;
        public const byte BYTE_STAT = 0x06;
        public const byte BYTE_STAT_ASYNC = 0x07;

        public const byte BYTE_CLOSE_FILE = 0x08;
        public const byte BYTE_CLOSE_FILE_ASYNC = 0x09;
        public const byte BYTE_SETPOS = 0x0A;
        public const byte BYTE_GETPOS = 0x0B;
        public const byte BYTE_STATFILE = 0x0C;
        public const byte BYTE_EOF = 0x0D;
        public const byte BYTE_READ_FILE = 0x0E;
        public const byte BYTE_READ_FILE_ASYNC = 0x0F;
        public const byte BYTE_CLOSE_DIR = 0x10;
        public const byte BYTE_CLOSE_DIR_ASYNC = 0x11;
        public const byte BYTE_GET_CWD = 0x12;
        public const byte BYTE_READ_DIR = 0x13;
        public const byte BYTE_READ_DIR_ASYNC = 0x14;

        public const byte BYTE_MOUNT_SD = 0x30;
        public const byte BYTE_MOUNT_SD_OK = 0x31;
        public const byte BYTE_MOUNT_SD_BAD = 0x32;

        // Other defines
        public const int FS_MAX_ENTNAME_SIZE = 256;
        public const int FS_MAX_ENTNAME_SIZE_PAD = 0;

        public const int FS_MAX_LOCALPATH_SIZE = 511;
        public const int FS_MAX_MOUNTPATH_SIZE = 128;

        // Logs folder
        public static string logs_root = "logs";
        
        static void Main(string[] args)
        {
            // Check if logs folder
            if (!Directory.Exists(logs_root))
            {
                Console.Error.WriteLine("Logs directory `{0}' does not exist!", logs_root);
                return;
            }
            // Delete logs
            System.IO.DirectoryInfo downloadedMessageInfo = new DirectoryInfo(logs_root);
            foreach (FileInfo file in downloadedMessageInfo.GetFiles())
            {
                file.Delete();
            }

            // Start server
            string name = "[listener]";
            try
            {
                TcpListener listener = new TcpListener(IPAddress.Any, 7332);
                listener.Start();
                Console.WriteLine(name + " Listening on 7332");

                int index = 0;
                while (true)
                {
                    TcpClient client = listener.AcceptTcpClient();
                    Console.WriteLine("connected");
                    Thread thread = new Thread(Handle);
                    thread.Name = "[" + index.ToString() + "]";
                    thread.Start(client);
                    index++;
                }
            }
            catch (Exception e)
            {
                Console.WriteLine(name + " " + e.Message);
            }
            Console.WriteLine(name + " Exit");
        }

        static void Log(StreamWriter log, String str)
        {
            log.WriteLine(str);
            log.Flush();
            Console.WriteLine(str);
        }

        static void Handle(object client_obj)
        {
            string name = Thread.CurrentThread.Name;
            StreamWriter log = null;

            try
            {
                TcpClient client = (TcpClient)client_obj;
                using (NetworkStream stream = client.GetStream())
                {
                    EndianBinaryReader reader = new EndianBinaryReader(stream);
                    EndianBinaryWriter writer = new EndianBinaryWriter(stream);

                    uint[] ids = reader.ReadUInt32s(4);

                    // Log connection
                    Console.WriteLine(name + " Accepted connection from client " + client.Client.RemoteEndPoint.ToString());
                    Console.WriteLine(name + " TitleID: " + ids[0].ToString("X8") + "-" + ids[1].ToString("X8"));

                    // Create log file for current thread
                    log = new StreamWriter(logs_root + "\\" + DateTime.Now.ToString("yyyy-MM-dd") + "-" + name + "-" + ids[0].ToString("X8") + "-" + ids[1].ToString("X8") + ".txt");
                    log.WriteLine(name + " Accepted connection from client " + client.Client.RemoteEndPoint.ToString());
                    log.WriteLine(name + " TitleID: " + ids[0].ToString("X8") + "-" + ids[1].ToString("X8"));

                    writer.Write(BYTE_SPECIAL);

                    while (true)
                    {
                        byte cmd_byte = reader.ReadByte();
                        switch (cmd_byte)
                        {
                            case BYTE_OPEN_FILE:
                            case BYTE_OPEN_FILE_ASYNC:
                                {
                                    int len_path = reader.ReadInt32();
                                    int len_mode = reader.ReadInt32();
                                    string path = reader.ReadString(Encoding.ASCII, len_path - 1);
                                    if (reader.ReadByte() != 0) throw new InvalidDataException();
                                    string mode = reader.ReadString(Encoding.ASCII, len_mode - 1);
                                    if (reader.ReadByte() != 0) throw new InvalidDataException();

                                    if (cmd_byte == BYTE_OPEN_FILE)
                                        Log(log, name + " FSOpenFile(\"" + path + "\", \"" + mode + "\")");
                                    else
                                        Log(log, name + " FSOpenFileAsync(\"" + path + "\", \"" + mode + "\")");

                                    break;
                                }
                            case BYTE_READ_FILE:
                            case BYTE_READ_FILE_ASYNC:
                                {
                                    int size = reader.ReadInt32();
                                    int count = reader.ReadInt32();
                                    int fd = reader.ReadInt32();

                                    if (cmd_byte == BYTE_READ_FILE)
                                        Log(log, name + " FSReadFile(size=" + size.ToString() + ", count=" + count.ToString() + ", fd=" + fd.ToString() + ")");
                                    else
                                        Log(log, name + " FSReadFileAsync(size=" + size.ToString() + ", count=" + count.ToString() + ", fd=" + fd.ToString() + ")");

                                    break;
                                }
                            case BYTE_CLOSE_FILE:
                            case BYTE_CLOSE_FILE_ASYNC:
                                {
                                    int fd = reader.ReadInt32();

                                    if (cmd_byte == BYTE_CLOSE_FILE)
                                        Log(log, name + " FSCloseFile(" + fd.ToString() + ")");
                                    else
                                        Log(log, name + " FSCloseFileAsync(" + fd.ToString() + ")");

                                    break;
                                }
                            case BYTE_SETPOS:
                                {
                                    int fd = reader.ReadInt32();
                                    int pos = reader.ReadInt32();

                                    Log(log, name + " FSSetPos(fd=" + fd.ToString() + ", pos=" + pos.ToString() + ")");

                                    break;
                                }
                            case BYTE_STATFILE:
                                {
                                    int fd = reader.ReadInt32();
                                    Log(log, name + " FSGetStatFile(" + fd.ToString() + ")");

                                    break;
                                }
                            case BYTE_OPEN_DIR:
                            case BYTE_OPEN_DIR_ASYNC:
                                {
                                    int len_path = reader.ReadInt32();
                                    string path = reader.ReadString(Encoding.ASCII, len_path - 1);
                                    if (reader.ReadByte() != 0) throw new InvalidDataException();

                                    if (cmd_byte == BYTE_OPEN_DIR)
                                        Log(log, name + " FSOpenDir(\"" + path + "\")");
                                    else
                                        Log(log, name + " FSOpenDirAsync(\"" + path + "\")");

                                    break;
                                }
                            case BYTE_READ_DIR:
                            case BYTE_READ_DIR_ASYNC:
                                {
                                    int fd = reader.ReadInt32();

                                    if (cmd_byte == BYTE_READ_DIR)
                                        Log(log, name + " FSReadDir(fd=" + fd.ToString() + ")");
                                    else
                                        Log(log, name + " FSReadDirAsync(fd=" + fd.ToString() + ")");

                                    break;
                                }
                            case BYTE_CLOSE_DIR:
                            case BYTE_CLOSE_DIR_ASYNC:
                                {
                                    int fd = reader.ReadInt32();

                                    if (cmd_byte == BYTE_CLOSE_DIR)
                                        Log(log, name + " FSCloseDir(" + fd.ToString() + ")");
                                    else
                                        Log(log, name + " FSCloseDirAsync(" + fd.ToString() + ")");

                                    break;
                                }
                            case BYTE_CHANGE_DIR:
                            case BYTE_CHANGE_DIR_ASYNC:
                                {
                                    int len_path = reader.ReadInt32();
                                    string path = reader.ReadString(Encoding.ASCII, len_path - 1);
                                    if (reader.ReadByte() != 0) throw new InvalidDataException();

                                    Log(log, name + " FSChangeDir(\"" + path + "\")");

                                    break;
                                }
                            case BYTE_GET_CWD:
                                {
                                    Log(log, name + " FSGetCwd()");

                                    break;
                                }
                            case BYTE_STAT:
                            case BYTE_STAT_ASYNC:
                                {
                                    int len_path = reader.ReadInt32();
                                    string path = reader.ReadString(Encoding.ASCII, len_path - 1);
                                    if (reader.ReadByte() != 0) throw new InvalidDataException();

                                    Log(log, name + " FSGetStat(\"" + path + "\")");
                                    break;
                                }
                            case BYTE_EOF:
                                {
                                    int fd = reader.ReadInt32();
                                    Log(log, name + " FSGetEof(" + fd.ToString() + ")");
                                    break;
                                }
                            case BYTE_GETPOS:
                                {
                                    int fd = reader.ReadInt32();
                                    Log(log, name + " FSGetPos(" + fd.ToString() + ")");
                                    break;
                                }
                            case BYTE_MOUNT_SD:
                                {
                                    Log(log, name + " Trying to mount SD card");
                                    break;
                                }
                            case BYTE_MOUNT_SD_OK:
                                {
                                    Log(log, name + " SD card mounted !");
                                    break;
                                }
                            case BYTE_MOUNT_SD_BAD:
                                {
                                    Log(log, name + " Can't mount SD card");
                                    break;
                                }
                            case BYTE_PING:
                                {
                                    int val1 = reader.ReadInt32();
                                    int val2 = reader.ReadInt32();

                                    Log(log, name + " PING RECEIVED with values : " + val1.ToString() + " - " + val2.ToString());
                                    break;
                                }
                            case BYTE_LOG_STR:
                                {
                                    int len_str = reader.ReadInt32();
                                    string str = reader.ReadString(Encoding.ASCII, len_str - 1);
                                    if (reader.ReadByte() != 0) throw new InvalidDataException();

                                    Log(log, name + " LogString =>(\"" + str + "\")");
                                    break;
                                }
                            default:
                                throw new InvalidDataException();
                        }
                    }
                }
            }
            catch (Exception e)
            {
                if (log != null)
                    Log(log, name + " " + e.Message);
                else
                    Console.WriteLine(name + " " + e.Message);
            }
            finally
            {
                if (log != null)
                    log.Close();
            }
            Console.WriteLine(name + " Exit");
        }
    }
}
