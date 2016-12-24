using System;
using System.IO;
using System.Net;
using System.Windows.Forms;
using System.Net.Sockets;
using System.Runtime.InteropServices;
using System.Text;
using System.Threading;
using System.Collections.Generic;

namespace saviine_server
{
    class Program
    {
        public const byte BYTE_NORMAL = 0xff;
        public const byte BYTE_SPECIAL = 0xfe;
        public const byte BYTE_OPEN = 0x00;
        public const byte BYTE_READ = 0x01;
        public const byte BYTE_CLOSE = 0x02;
        public const byte BYTE_OK = 0x03;
        public const byte BYTE_SETPOS = 0x04;
        //public const byte BYTE_STATFILE = 0x05;
        //public const byte BYTE_EOF = 0x06;
        //public const byte BYTE_GETPOS = 0x07;
        public const byte BYTE_REQUEST = 0x08;
        public const byte BYTE_REQUEST_SLOW = 0x09;
        public const byte BYTE_HANDLE = 0x0A;
        public const byte BYTE_DUMP = 0x0B;
        public const byte BYTE_PING = 0x0C;
        public const byte BYTE_G_MODE = 0x0D;
        public const byte BYTE_MODE_D = 0x0E;
        public const byte BYTE_MODE_I = 0x0F;
        public const byte BYTE_CLOSE_DUMP = 0x10;
        public const byte BYTE_LOG_STR = 0xFB;
        public const byte BYTE_FILE = 0xC0;
        public const byte BYTE_FOLDER = 0xC1;
        public const byte BYTE_READ_DIR = 0xCC;
        public const byte BYTE_INJECTSTART = 0x40;
        public const byte BYTE_INJECTEND = 0x41;
        public const byte BYTE_DUMPSTART = 0x42;
        public const byte BYTE_DUMPEND = 0x43;
        public const byte BYTE_END = 0xfd;


        public const int MASK_NORMAL = 0x8000;
        public const int MASK_USER = 0x0100;
        public const int MASK_COMMON = 0x0200;
        public const int MASK_COMMON_CLEAN = 0x0400;
        

        private static long currentPersistentID = 0x0;
        private static long COMMON_PERSISTENTID = 0x1;
        

        [Flags]
        public enum FSStatFlag : uint
        {
            None = 0,
            unk_14_present = 0x01000000,
            mtime_present = 0x04000000,
            ctime_present = 0x08000000,
            entid_present = 0x10000000,
            directory = 0x80000000,
        }

        [StructLayout(LayoutKind.Sequential)]
        public struct FSStat
        {
            public FSStatFlag flags;
            public uint permission;
            public uint owner;
            public uint group;
            public uint file_size;
            public uint unk_14_nonzero;
            public uint unk_18_zero;
            public uint unk_1c_zero;
            public uint ent_id;
            public uint ctime_u;
            public uint ctime_l;
            public uint mtime_u;
            public uint mtime_l;
            public uint unk_34_zero;
            public uint unk_38_zero;
            public uint unk_3c_zero;
            public uint unk_40_zero;
            public uint unk_44_zero;
            public uint unk_48_zero;
            public uint unk_4c_zero;
            public uint unk_50_zero;
            public uint unk_54_zero;
            public uint unk_58_zero;
            public uint unk_5c_zero;
            public uint unk_60_zero;
        }

        public static string root = "saviine_root";
        public static string logs_root = "logs";
        public static string injectfolder = "inject";
        public static string dumpfolder = "dump";
        public static string common = "common";

        const uint BUFFER_SIZE = 64 * 1024;
        static Boolean fastmode = false;
        static byte op_mode = BYTE_MODE_D;
         [STAThread]
        static void Main(string[] args)
        {          
            if (args.Length > 1)
            {
                Console.Error.WriteLine("Usage: saviine_server [fastmode|fast]");
                return;
            }
            if (args.Length == 1)
            {
                if (args[0].Equals("fastmode") || args[0].Equals("fast") || args[0].Equals("-fast"))
                {
                    op_mode = BYTE_MODE_D;
                    fastmode = true;                   
                }else if(args[0].Equals("inject")) {
                    op_mode = BYTE_MODE_I;
                }
                         
            }
            if (args.Length == 2)
            {
                if (args[0].Equals("dump")) {
                    op_mode = BYTE_MODE_D;                   
                    if (args[1].Equals("fastmode") || args[1].Equals("fast") || args[1].Equals("-fast"))
                    {
                        fastmode = true;                        
                    }
                }else if(args[0].Equals("inject")) {
                    op_mode = BYTE_MODE_I;
                }
            }

            if (op_mode == BYTE_MODE_D)
            {
                currentPersistentID = 0x01;
                Console.WriteLine("Dump mode");
                if(fastmode)Console.WriteLine("Now using fastmode");
            }
            else if(op_mode == BYTE_MODE_I)
            {
                Console.WriteLine("Injection mode!");
            }

            // Check for cafiine_root and logs folder
            if (!Directory.Exists(root))
            {
                Console.Error.WriteLine("Root directory `{0}' does not exist!", root);
								Directory.CreateDirectory(root);
								Console.WriteLine("Root directory `{0}' created!", root);
            }
            if (!Directory.Exists(logs_root))
            {
                Console.Error.WriteLine("Logs directory `{0}' does not exist!", logs_root);
								Directory.CreateDirectory(logs_root);
								Console.WriteLine("Logs directory `{0}' created!", logs_root);
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
        static void LogNoLine(StreamWriter log, String str)
        {
            log.Write(str);
            log.Flush();
            Console.Write(str);
        }
        public static int countDirectory(string targetDirectory) 
        {
                int x = 0;
            // Process the list of files found in the directory.
            string [] fileEntries = Directory.GetFiles(targetDirectory);
            foreach(string fileName in fileEntries)
                x++;

            // Recurse into subdirectories of this directory.
            string [] subdirectoryEntries = Directory.GetDirectories(targetDirectory);
            foreach(string subdirectory in subdirectoryEntries)
                x++;

            return x;
        }
                     

        static void Handle(object client_obj)
        {
            string name = Thread.CurrentThread.Name;
            FileStream[] files = new FileStream[256];
            Dictionary<int, FileStream> files_request = new Dictionary<int, FileStream>();
            StreamWriter log = null;
            Dictionary<string, Dictionary<string, byte>> dir_files = new Dictionary<string, Dictionary<string, byte>>();


            try
            {
                TcpClient client = (TcpClient)client_obj;
                using (NetworkStream stream = client.GetStream())
                {
                    EndianBinaryReader reader = new EndianBinaryReader(stream);
                    EndianBinaryWriter writer = new EndianBinaryWriter(stream);

                    uint[] ids = reader.ReadUInt32s(4);




                    string LocalRootDump = root + "\\" + "dump" + "\\" + ids[0].ToString("X8") + "-" + ids[1].ToString("X8") + "\\";
                   string LocalRootInject = root + "\\" + "inject" + "\\" + ids[0].ToString("X8") + "-" + ids[1].ToString("X8") + "\\";

                   if (!ids[0].ToString("X8").Equals("00050000"))
                    {
                        writer.Write(BYTE_NORMAL);
                        throw new Exception("Not interested.");
                    }
                   else
                   {
                       if (!Directory.Exists(LocalRootDump))
                       {
                           Directory.CreateDirectory(LocalRootDump);
                       }
                       if (!Directory.Exists(LocalRootInject))
                       {
                           Directory.CreateDirectory(LocalRootInject);
                       }
                   }
                   // Log connection
                   Console.WriteLine(name + " Accepted connection from client " + client.Client.RemoteEndPoint.ToString());
                   Console.WriteLine(name + " TitleID: " + ids[0].ToString("X8") + "-" + ids[1].ToString("X8"));

                    // Create log file for current thread
                    log = new StreamWriter(logs_root + "\\" + DateTime.Now.ToString("yyyy-MM-dd") + "-" + name + "-" + ids[0].ToString("X8") + "-" + ids[1].ToString("X8") + ".txt", true, Encoding.ASCII, 1024*64);
                    log.WriteLine(name + " Accepted connection from client " + client.Client.RemoteEndPoint.ToString());
                    string title_id = ids[0].ToString("X8") + "-" + ids[1].ToString("X8");
                    log.WriteLine(name + " TitleID: " + title_id);                   

                    writer.Write(BYTE_SPECIAL);

                    while (true)
                    {
                        //Log(log, "cmd_byte");
                        byte cmd_byte = reader.ReadByte();                        
                        switch (cmd_byte)
                        {
                            case BYTE_OPEN:
                                {
                                    //Log(log, "BYTE_OPEN");
                                    Boolean failed = false;

                                    int len_path = reader.ReadInt32();
                                    int len_mode = reader.ReadInt32();
                                    string path = reader.ReadString(Encoding.ASCII, len_path - 1);
                                    if (reader.ReadByte() != 0) throw new InvalidDataException();
                                    string mode = reader.ReadString(Encoding.ASCII, len_mode - 1);
                                    if (reader.ReadByte() != 0) throw new InvalidDataException();

                                   
                                    //Log(log, "old path" + path);
                                    //Log(log, "currentID: " + currentPersistentID);
                                    if (op_mode == BYTE_MODE_I)
                                        path = getRealPathCurrentInject(path, title_id);
                                  
                                    //Log(log, "new path" + path);
                                    if (path.Length == 0) failed = true;

                                    if (File.Exists(path) && !failed)
                                    {
                                        //Log(log, "path exits");
                                        int handle = -1;
                                        for (int i = 1; i < files.Length; i++)
                                        {
                                            if (files[i] == null)
                                            {
                                                handle = i;
                                                break;
                                            }
                                        }
                                        if (handle == -1)
                                        {
                                            Log(log, name + " Out of file handles!");
                                            writer.Write(BYTE_SPECIAL);
                                            writer.Write(-19);
                                            writer.Write(0);
                                            break;
                                        }
                                        //Log(log, name + " -> fopen(\"" + path + "\", \"" + mode + "\") = " + handle.ToString());

                                        files[handle] = new FileStream(path, FileMode.Open, FileAccess.Read, FileShare.Read);

                                        writer.Write(BYTE_SPECIAL);
                                        writer.Write(0);
                                        writer.Write(handle);
                                        break;
                                    }
                                    //Log(log, "error fopen");
                                    //else on error:
                                    writer.Write(BYTE_NORMAL); 

                                    break;
                                }
                            case BYTE_SETPOS:
                                {
                                    //Log(log, "BYTE_SETPOS");
                                    int fd = reader.ReadInt32();
                                    int pos = reader.ReadInt32();
                                    if ((fd & 0x0fff00ff) == 0x0fff00ff)
                                    {
                                        int handle = (fd >> 8) & 0xff;
                                        if (files[handle] == null)
                                        {
                                            writer.Write(BYTE_SPECIAL);
                                            writer.Write(-38);
                                            break;
                                        }
                                        FileStream f = files[handle];
                                        Log(log, "Postion was set to " + pos + "for handle " + handle);
                                        f.Position = pos;
                                        writer.Write(BYTE_SPECIAL);
                                        writer.Write(0);
                                    }
                                    else
                                    {
                                        writer.Write(BYTE_NORMAL);
                                    }
                                    break;
                                }
                            case BYTE_INJECTSTART:
                                {
                                    long wiiUpersistentID = (long)reader.ReadUInt32();
                                    int dumpCommon = 0;
                                    Boolean injectioncanceled = false;
                                    SaveSelectorDialog ofd = new SaveSelectorDialog(title_id, wiiUpersistentID);
                                    try
                                    {
                                        DialogResult result = ofd.ShowDialog();
                                        if (result == System.Windows.Forms.DialogResult.OK)
                                        {
                                            currentPersistentID = ofd.NewPersistentID;
                                            dumpCommon = ofd.DumpCommon;
                                            //Console.WriteLine("Injecting " + currentPersistentID.ToString() + " into " + wiiUpersistentID.ToString() + " for title id " + title_id);
                                            if (dumpCommon == 1) Console.WriteLine("clean and inject common folder");
                                            if (dumpCommon == 2) Console.WriteLine("inject common folder");
                                            if (dumpCommon > 0 && currentPersistentID == 0) currentPersistentID = COMMON_PERSISTENTID;
                                        }
                                        else
                                        {
                                            Console.WriteLine("Injection canceled");
                                            injectioncanceled = true;
                                        }
                                    }
                                    catch (Exception e)
                                    {
                                        Console.WriteLine("Injection canceled");
                                        injectioncanceled = true;
                                    }
                                    if (injectioncanceled)
                                    {
                                        writer.Write(BYTE_NORMAL);
                                    }
                                    else
                                    {
                                        writer.Write(BYTE_SPECIAL);
                                    }
                                    int dumpmask = MASK_NORMAL;
                                    if (currentPersistentID != 0 && currentPersistentID != COMMON_PERSISTENTID)
                                        dumpmask |= MASK_USER;

                                    if (dumpCommon >= 1) {
                                        dumpmask |= MASK_COMMON;
                                      if(dumpCommon == 2)
                                        dumpmask |= MASK_COMMON_CLEAN;
                                    }
                                    writer.Write(dumpmask);
                                    writer.Write(BYTE_SPECIAL);

                                    break;
                                }
                            case BYTE_INJECTEND:
                                {
                                    currentPersistentID = 0;
                                    //close all opened files
                                    for (int i = 1; i < files.Length; i++)
                                    {
                                        if (files[i] != null)
                                        {
                                            files[i].Close();
                                            files[i] = null;
                                        }
                                    }
                                    writer.Write(BYTE_OK);
                                    //Console.WriteLine("InjectionEND");
                                        
                                    break;
                                }
                            case BYTE_DUMPSTART:
                                {
                                    long wiiUpersistentID = (long)reader.ReadUInt32();
                                    Boolean dumpCommon = false;
                                    Boolean dumpUser = false;
                                    currentPersistentID = wiiUpersistentID;                                   
                                    
                                    Boolean dumpcanceled = false;
                                    DumpDialog ofd = new DumpDialog(title_id, wiiUpersistentID);
                                    try
                                    {
                                        DialogResult result = ofd.ShowDialog();
                                        if (result == System.Windows.Forms.DialogResult.OK)
                                        {
                                            dumpUser = ofd.DumpUser;
                                            dumpCommon = ofd.DumpCommon;
                                            //Console.WriteLine("Injecting " + currentPersistentID.ToString() + " into " + wiiUpersistentID.ToString() + " for title id " + title_id);
                                            if (dumpCommon) Console.WriteLine("dumping common data");
                                            if (dumpUser) Console.WriteLine("dumping user data");                                            
                                        }
                                        else
                                        {
                                            Console.WriteLine("dump canceled");
                                            dumpcanceled = true;
                                        }
                                    }
                                    catch (Exception e)
                                    {
                                        Console.WriteLine("dump canceled");
                                        dumpcanceled = true;
                                    }
                                    if (dumpcanceled)
                                    {
                                        writer.Write(BYTE_NORMAL);
                                    }
                                    else
                                    {
                                        writer.Write(BYTE_SPECIAL);
                                    }
                                   
                                    int dumpmask = MASK_NORMAL;
                                    if (dumpUser)
                                        dumpmask |= MASK_USER;

                                    if (dumpCommon)
                                    {
                                        dumpmask |= MASK_COMMON;                                        
                                    }
                                    writer.Write(dumpmask);
                                    writer.Write(BYTE_SPECIAL);

                                    break;
                                }
                            case BYTE_DUMPEND:
                                {
                                    currentPersistentID = 0;
                                    //close all opened files
                                    for (int i = 1; i < files.Length; i++)
                                    {
                                        if (files[i] != null)
                                        {
                                            files[i].Close();
                                            files[i] = null;
                                        }
                                    }
                                    writer.Write(BYTE_OK);
                                    //Console.WriteLine("dumpEND");

                                    break;
                                }
                            case BYTE_READ_DIR: 
                                {                                    
                                    Boolean failed = false;
                                    int len_path = reader.ReadInt32();
                                    string path = reader.ReadString(Encoding.ASCII, len_path-1);
                                    if (reader.ReadByte() != 0) throw new InvalidDataException();
                                    int x = 0;
                                    //Console.WriteLine("old" + path);
                                    if(op_mode == BYTE_MODE_I)
                                        path = getRealPathCurrentInject(path, title_id);
                                    //Console.WriteLine("new" + path);
                                    if(path.Length == 0)failed = true;
                                  
                                    if (Directory.Exists(path) && !failed)
                                    {
                                        x = countDirectory(path);
                                        if (x > 0)
                                        {
                                            Dictionary<string, byte> value;
                                            if (!dir_files.TryGetValue(path, out value))
                                            {
                                                //Console.Write("found no \"" + path + "\" in dic \n");
                                                value = new Dictionary<string, byte>();
                                                string[] fileEntries = Directory.GetFiles(path);
                                                foreach (string fn in fileEntries)
                                                {
                                                    string fileName = Path.GetFileName(fn);
                                                    value.Add(fileName, BYTE_FILE);
                                                }
                                                string[] subdirectoryEntries = Directory.GetDirectories(path);
                                                foreach (string sd in subdirectoryEntries)
                                                {
                                                    string subdirectory = Path.GetFileName(sd);
                                                    value.Add(subdirectory, BYTE_FOLDER);
                                                }
                                                dir_files.Add(path, value);
                                                //Console.Write("added \"" + path + "\" to dic \n");
                                            }
                                            else
                                            {
                                                //Console.Write("dic for \"" + path + "\" ready \n");
                                            }

                                            if (value.Count > 0)
                                            {
                                                writer.Write(BYTE_OK);
                                                //Console.Write("sent ok byte \n");
                                                foreach (var item in value)
                                                { //Write 
                                                    writer.Write(item.Value);
                                                    //Console.Write("type : " + item.Value);
                                                    writer.Write(item.Key.Length);
                                                    //Console.Write("length : " + item.Key.Length);
                                                    writer.Write(item.Key, Encoding.ASCII, true);
                                                    //Console.Write("filename : " + item.Key);
                                                    int length = 0;
                                                    if (item.Value == BYTE_FILE) length = (int)new System.IO.FileInfo(path + "/" + item.Key).Length;
                                                    writer.Write(length);
                                                    //Console.Write("filesize : " + length + " \n");
                                                    value.Remove(item.Key);
                                                    //Console.Write("removed from list! " + value.Count + " remaining\n");
                                                    break;
                                                }
                                                writer.Write(BYTE_SPECIAL); //
                                                //Console.Write("file sent, wrote special byte \n");
                                                break;
                                            }
                                            else
                                            {

                                                dir_files.Remove(path);
                                                //Console.Write("removed \"" + path + "\" from dic \n");
                                            }
                                        }
                                    }
                                    writer.Write(BYTE_END); //
                                    //Console.Write("list was empty return BYTE_END \n");

                                    //Console.Write("in break \n");
                                    break;
                                }    
                            case BYTE_READ:
                                {
                                    //Log(log,"BYTE_READ");
                                    int size = reader.ReadInt32();                                   
                                    int fd = reader.ReadInt32();

                                    FileStream f = files[fd];

                                    byte[] buffer = new byte[size];
                                    int sz = (int)f.Length;                                      
                                    int rd = 0;

                                    //Log(log, "want size:" + size + " for handle: " + fd);

                                    writer.Write(BYTE_SPECIAL);        
                                        
                                    rd = f.Read(buffer, 0, buffer.Length);
                                    //Log(log,"rd:" + rd);                                  
                                    writer.Write(rd);   
                                    writer.Write(buffer, 0, rd);

                                    int offset = (int)f.Position;
                                    int progress = (int)(((float)offset / (float)sz) * 100);
                                    string strProgress = progress.ToString().PadLeft(3, ' ');
                                    string strSize = (sz / 1024).ToString();
                                    string strCurrent = (offset / 1024).ToString().PadLeft(strSize.Length, ' ');
                                    Console.Write("\r\t--> {0}% ({1} kB / {2} kB)", strProgress, strCurrent, strSize);
                                    
                                    //Console.Write("send " + rd );                                           
                                    if (offset == sz)
                                    {
                                        Console.Write("\n");
                                        log.Write("\r\t--> {0}% ({1} kB / {2} kB)\n", strProgress, strCurrent, strSize);
                                    }
                                    int ret = -5;
                                    if ((ret =reader.ReadByte()) != BYTE_OK)
                                    {
                                        Console.Write("error, got " + ret + " instead of " + BYTE_OK);
                                        //throw new InvalidDataException();
                                    }
                                       
                                    //Log(log, "break READ");

                                    break;
                                }
                            case BYTE_HANDLE:
                                {                
                                    //Log(log,"BYTE_HANDLE");
                                    // Read buffer params : fd, path length, path string
                                    int fd = reader.ReadInt32();
                                    int len_path = reader.ReadInt32();
                                    string path = reader.ReadString(Encoding.ASCII, len_path - 1);
                                    if (reader.ReadByte() != 0) throw new InvalidDataException();
                                    //Console.WriteLine("old " + path);
                                    if (op_mode == BYTE_MODE_D)
                                        path = getRealPathCurrentDump(path, title_id);
                                    //Console.WriteLine("new " + path);

                                    if (path.Length == 0)
                                    {
                                        writer.Write(BYTE_SPECIAL);
                                        break;
                                    }

                                    if (!Directory.Exists(path))
                                    {
                                        Directory.CreateDirectory(Path.GetDirectoryName(path));
                                    }

                                    // Add new file for incoming data
                                    files_request.Add(fd, new FileStream(path, FileMode.OpenOrCreate, FileAccess.Write, FileShare.Write));
                                    // Send response
                                    if (fastmode) {                                       
                                        writer.Write(BYTE_REQUEST);
                                    }
                                    else
                                    {                                        
                                        writer.Write(BYTE_REQUEST_SLOW);
                                    }
                                    LogNoLine(log, "-> [");
                                    // Send response
                                    writer.Write(BYTE_SPECIAL);
                                    break;
                                }
                            case BYTE_DUMP:
                                {               
                                    //Log(log,"BYTE_DUMP");
                                    // Read buffer params : fd, size, file data
                                    int fd = reader.ReadInt32();
                                    int sz = reader.ReadInt32();
                                    byte[] buffer = new byte[sz];
                                    buffer = reader.ReadBytes(sz);

                                    // Look for file descriptor
                                    foreach (var item in files_request)
                                    {
                                        if (item.Key == fd)
                                        {
                                            FileStream dump_file = item.Value;
                                            if (dump_file == null)
                                                break;
                                            
                                            LogNoLine(log, ".");

                                            // Write to file
                                            dump_file.Write(buffer, 0, sz);

                                            break;
                                        }
                                    }                                    
                                    // Send response
                                    writer.Write(BYTE_SPECIAL);
                                    break;
                                }                            
                            case BYTE_CLOSE:
                                {
                                    //Log(log, "BYTE_CLOSE");
                                    int fd = reader.ReadInt32();
                                
                                   
                                    if (files[fd] == null)
                                    {
                                        writer.Write(BYTE_SPECIAL);
                                        writer.Write(-38);
                                        break;
                                    }
                                    //Log(log, name + " close(" + fd.ToString() + ")");
                                    FileStream f = files[fd];

                                    writer.Write(BYTE_SPECIAL);
                                    writer.Write(0);
                                    f.Close();
                                    files[fd] = null;
                                    
                                   
                                    break;
                                }
                            case BYTE_CLOSE_DUMP:                               
                                {
                                    int fd = reader.ReadInt32();
                                    if ((fd & 0x0fff00ff) != 0x0fff00ff)
                                    {
                                        // Check if it is a file to dump
                                        foreach (var item in files_request)
                                        {
                                            if (item.Key == fd)
                                            {
                                                FileStream dump_file = item.Value;
                                                if (dump_file == null)
                                                    break;

                                                LogNoLine(log,"]");
                                                Log(log, "");
                                                // Close file and remove from request list
                                                dump_file.Close();
                                                files_request.Remove(fd);
                                                break;
                                            }
                                        }

                                        // Send response
                                        writer.Write(BYTE_NORMAL);
                                    }
                                    break;
                                }          
                            case BYTE_PING:
                                {
                                    //Log(log, "BYTE_PING");
                                    int val1 = reader.ReadInt32();
                                    int val2 = reader.ReadInt32();

                                    Log(log, name + " PING RECEIVED with values : " + val1.ToString() + " - " + val2.ToString());
                                    break;
                                }
                            case BYTE_G_MODE:
                                {
                                    if (op_mode == BYTE_MODE_D)
                                    {
                                        writer.Write(BYTE_MODE_D);
                                    }
                                    else if (op_mode == BYTE_MODE_I)
                                    {
                                        writer.Write(BYTE_MODE_I);
                                    }                                    
                                    break;
                                }

                            case BYTE_LOG_STR:
                                {
                                    //Log(log, "BYTE_LOG_STR");
                                    int len_str = reader.ReadInt32();
                                    string str = reader.ReadString(Encoding.ASCII, len_str - 1);
                                    if (reader.ReadByte() != 0) throw new InvalidDataException();

                                    Log(log,"-> " + str);
                                    break;
                                }
                            default:
                                Log(log, "xx" + cmd_byte);
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
                foreach (var item in files)
                {
                    if (item != null)
                        item.Close();
                }
                foreach (var item in files_request)
                {
                    if (item.Value != null)
                        item.Value.Close();
                }

                if (log != null)
                    log.Close();
            }
            Console.WriteLine(name + " Exit");
        }

        private static string getRealPathCurrentInject(string path, string title_id)
        {
            return getRealPathCurrent(injectfolder, path, title_id);
        }
        private static string getRealPathCurrentDump(string path, string title_id)
        {
            return getRealPathCurrent(dumpfolder, path, title_id);
        }

        private static string getRealPathCurrent(string prefix, string path, string title_id)
        {
            string savePath = Program.root + "/" + prefix + "/" + title_id;
            if (currentPersistentID == 0) return "";           
            string[] stringSeparators = new string[] { "vol/save/", "vol\\save\\" };
            string[] result;
            string resultstr = "";           
            result = path.Split(stringSeparators, StringSplitOptions.None);
            if (result.Length < 2) return "";            
            resultstr = result[result.Length-1];           
            stringSeparators = new string[] { "/", "\\" };
            result = resultstr.Split(stringSeparators, StringSplitOptions.None);
            if (result.Length < 2)
            {
                if (result[0] != "common") return savePath  + "/" + String.Format("{0:X}", currentPersistentID);
                return savePath + "/" + "common";
            }
            resultstr = "";
            if (result[0] != "common")
                savePath += "/" + String.Format("{0:X}", currentPersistentID);
            else
                savePath += "/" + "common";
            for (int i = 1; i < result.Length; i++)
            {
                resultstr += "/" + result[i];
            }
           
            savePath += resultstr;

            return savePath;
        }
    }
}
