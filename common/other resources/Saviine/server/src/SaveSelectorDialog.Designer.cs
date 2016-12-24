namespace saviine_server
{
    partial class SaveSelectorDialog
    {
        /// <summary>
        /// Required designer variable.
        /// </summary>
        private System.ComponentModel.IContainer components = null;

        /// <summary>
        /// Clean up any resources being used.
        /// </summary>
        /// <param name="disposing">true if managed resources should be disposed; otherwise, false.</param>
        protected override void Dispose(bool disposing)
        {
            if (disposing && (components != null))
            {
                components.Dispose();
            }
            base.Dispose(disposing);
        }

        #region Windows Form Designer generated code

        /// <summary>
        /// Required method for Designer support - do not modify
        /// the contents of this method with the code editor.
        /// </summary>
        private void InitializeComponent()
        {
            this.btn_ok = new System.Windows.Forms.Button();
            this.btn_cancel = new System.Windows.Forms.Button();
            this.lbl_message = new System.Windows.Forms.Label();
            this.comBoxIDList = new System.Windows.Forms.ComboBox();
            this.label1 = new System.Windows.Forms.Label();
            this.Inj = new System.Windows.Forms.Label();
            this.comBoxCommon = new System.Windows.Forms.ComboBox();
            this.SuspendLayout();
            // 
            // btn_ok
            // 
            this.btn_ok.DialogResult = System.Windows.Forms.DialogResult.OK;
            this.btn_ok.Location = new System.Drawing.Point(62, 133);
            this.btn_ok.Name = "btn_ok";
            this.btn_ok.Size = new System.Drawing.Size(81, 21);
            this.btn_ok.TabIndex = 0;
            this.btn_ok.Text = "OK";
            this.btn_ok.UseVisualStyleBackColor = true;
            this.btn_ok.Click += new System.EventHandler(this.btn_ok_Click);
            // 
            // btn_cancel
            // 
            this.btn_cancel.DialogResult = System.Windows.Forms.DialogResult.Cancel;
            this.btn_cancel.Location = new System.Drawing.Point(198, 133);
            this.btn_cancel.Name = "btn_cancel";
            this.btn_cancel.Size = new System.Drawing.Size(81, 21);
            this.btn_cancel.TabIndex = 1;
            this.btn_cancel.Text = "Cancel";
            this.btn_cancel.UseVisualStyleBackColor = true;
            // 
            // lbl_message
            // 
            this.lbl_message.AutoSize = true;
            this.lbl_message.Location = new System.Drawing.Point(12, 10);
            this.lbl_message.Name = "lbl_message";
            this.lbl_message.Size = new System.Drawing.Size(236, 13);
            this.lbl_message.TabIndex = 3;
            this.lbl_message.Text = "Got an injection request for 00050000-10157F00";
            // 
            // comBoxIDList
            // 
            this.comBoxIDList.FormattingEnabled = true;
            this.comBoxIDList.Location = new System.Drawing.Point(186, 48);
            this.comBoxIDList.Name = "comBoxIDList";
            this.comBoxIDList.Size = new System.Drawing.Size(93, 21);
            this.comBoxIDList.TabIndex = 4;
            // 
            // label1
            // 
            this.label1.AutoSize = true;
            this.label1.Location = new System.Drawing.Point(12, 51);
            this.label1.Name = "label1";
            this.label1.Size = new System.Drawing.Size(81, 13);
            this.label1.TabIndex = 5;
            this.label1.Text = "Select userdata";
            this.label1.Click += new System.EventHandler(this.label1_Click);
            // 
            // Inj
            // 
            this.Inj.AutoSize = true;
            this.Inj.Location = new System.Drawing.Point(12, 86);
            this.Inj.Name = "Inj";
            this.Inj.Size = new System.Drawing.Size(108, 13);
            this.Inj.TabIndex = 7;
            this.Inj.Text = "Inject common save?";
            this.Inj.Click += new System.EventHandler(this.Inj_Click);
            // 
            // comBoxCommon
            // 
            this.comBoxCommon.FormattingEnabled = true;
            this.comBoxCommon.Items.AddRange(new object[] {
            "no",
            "inject",
            "clean and inject (deleting existing common)"});
            this.comBoxCommon.Location = new System.Drawing.Point(126, 83);
            this.comBoxCommon.Name = "comBoxCommon";
            this.comBoxCommon.Size = new System.Drawing.Size(153, 21);
            this.comBoxCommon.TabIndex = 8;
            // 
            // SaveSelectorDialog
            // 
            this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
            this.ClientSize = new System.Drawing.Size(310, 166);
            this.Controls.Add(this.comBoxCommon);
            this.Controls.Add(this.Inj);
            this.Controls.Add(this.label1);
            this.Controls.Add(this.comBoxIDList);
            this.Controls.Add(this.lbl_message);
            this.Controls.Add(this.btn_cancel);
            this.Controls.Add(this.btn_ok);
            this.FormBorderStyle = System.Windows.Forms.FormBorderStyle.FixedDialog;
            this.Name = "SaveSelectorDialog";
            this.Text = "Injection request";
            this.Load += new System.EventHandler(this.Form1_Load);
            this.ResumeLayout(false);
            this.PerformLayout();

        }

        #endregion

        private System.Windows.Forms.Button btn_ok;
        private System.Windows.Forms.Button btn_cancel;
        private System.Windows.Forms.Label lbl_message;
        private System.Windows.Forms.ComboBox comBoxIDList;
        private System.Windows.Forms.Label label1;
        private System.Windows.Forms.Label Inj;
        private System.Windows.Forms.ComboBox comBoxCommon;
    }
}