using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Drawing;
using System.IO;
using System.Linq;
using System.Text;
using System.Windows.Forms;

namespace saviine_server
{
    public partial class SaveSelectorDialog : Form
    {
        private long newPersistentID = 0;
        private int dumpCommon = 0;

        public long NewPersistentID
        {
            get { return newPersistentID; }
        }
        public int DumpCommon
        {
            get { return dumpCommon; }
        }
        private static string savePath; 
        public SaveSelectorDialog(string title_id,long persistentID)
        {
            InitializeComponent();
            comBoxCommon.SelectedIndex = 0;
            savePath = Program.root + "/" + Program.injectfolder;;
            this.lbl_message.Text = "Got an injection request for " + title_id;
            savePath += "/" + title_id;
            string[] subdirectoryEntries;
            if (Directory.Exists(savePath))
            {
                // Recurse into subdirectories of this directory.
                subdirectoryEntries = Directory.GetDirectories(savePath);
                this.comBoxIDList.Items.Add("---none---");
                comBoxIDList.SelectedIndex = 0;
                foreach (string subdirectory in subdirectoryEntries)
                {
                    string filename = Path.GetFileName(subdirectory);
                    long id;
                    try{
                        id = Convert.ToUInt32(filename, 16);
                    }catch (Exception){
                        id = 0;
                    }
                    
                    
                    if (id >= 0x80000000 && id <= 0x81000000)
                    {

                        this.comBoxIDList.Items.Add(filename);
                    }                   
                }
                if (comBoxIDList.Items.Count == 1)
                {
                    this.comBoxIDList.Enabled = false;

                }
                if (!Directory.Exists(savePath + "/" + Program.common))
                {
                    comBoxCommon.Enabled = false;
                }
            }


        }

        private void Form1_Load(object sender, EventArgs e)
        {

        }

        private void btn_ok_Click(object sender, EventArgs e)
        {
            long id;
            try
            {
                id = Convert.ToUInt32(this.comBoxIDList.SelectedItem.ToString(), 16);
            }
            catch (Exception)
            {
                id = 0;
            }
            newPersistentID = id;
            dumpCommon = comBoxCommon.SelectedIndex;
            Console.WriteLine(dumpCommon);
            
        }
        private void btn_cancel_Click(object sender, EventArgs e)
        {
            
        }

        private void Inj_Click(object sender, EventArgs e)
        {

        }

        private void label1_Click(object sender, EventArgs e)
        {

        }
        
      
    }
}
