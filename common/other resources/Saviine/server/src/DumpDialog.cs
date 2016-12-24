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
    public partial class DumpDialog : Form
    {
        private Boolean dumpUser = false;
        private Boolean dumpCommon = false;

        public Boolean DumpCommon
        {
            get { return dumpCommon; }
        }
        public Boolean DumpUser
        {
            get { return dumpUser; }
        }
        private static string savePath;
        public DumpDialog(string title_id, long persistentID)
        {
            InitializeComponent();         

        }

     
        private void btn_ok_Click(object sender, EventArgs e)
        {
            dumpUser = checkBoxUser.Checked;
            dumpCommon = checkBoxCommon.Checked;           
            
        }
      
    }
}
