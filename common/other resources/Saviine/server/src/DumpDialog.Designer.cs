namespace saviine_server
{
    partial class DumpDialog
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
            this.label1 = new System.Windows.Forms.Label();
            this.Inj = new System.Windows.Forms.Label();
            this.checkBoxUser = new System.Windows.Forms.CheckBox();
            this.checkBoxCommon = new System.Windows.Forms.CheckBox();
            this.SuspendLayout();
            // 
            // btn_ok
            // 
            this.btn_ok.DialogResult = System.Windows.Forms.DialogResult.OK;
            this.btn_ok.Location = new System.Drawing.Point(15, 133);
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
            this.btn_cancel.Location = new System.Drawing.Point(148, 133);
            this.btn_cancel.Name = "btn_cancel";
            this.btn_cancel.Size = new System.Drawing.Size(81, 21);
            this.btn_cancel.TabIndex = 1;
            this.btn_cancel.Text = "Cancel";
            this.btn_cancel.UseVisualStyleBackColor = true;
            // 
            // lbl_message
            // 
            this.lbl_message.AutoSize = true;
            this.lbl_message.Location = new System.Drawing.Point(12, 9);
            this.lbl_message.Name = "lbl_message";
            this.lbl_message.Size = new System.Drawing.Size(217, 13);
            this.lbl_message.TabIndex = 3;
            this.lbl_message.Text = "Got a dump request";
            // 
            // label1
            // 
            this.label1.AutoSize = true;
            this.label1.Location = new System.Drawing.Point(12, 51);
            this.label1.Name = "label1";
            this.label1.Size = new System.Drawing.Size(84, 13);
            this.label1.TabIndex = 5;
            this.label1.Text = "Dump user save";
            // 
            // Inj
            // 
            this.Inj.AutoSize = true;
            this.Inj.Location = new System.Drawing.Point(12, 86);
            this.Inj.Name = "Inj";
            this.Inj.Size = new System.Drawing.Size(104, 13);
            this.Inj.TabIndex = 7;
            this.Inj.Text = "Dump common save";
            // 
            // checkBoxUser
            // 
            this.checkBoxUser.AutoSize = true;
            this.checkBoxUser.Checked = true;
            this.checkBoxUser.CheckState = System.Windows.Forms.CheckState.Checked;
            this.checkBoxUser.Location = new System.Drawing.Point(202, 50);
            this.checkBoxUser.Name = "checkBoxUser";
            this.checkBoxUser.Size = new System.Drawing.Size(15, 14);
            this.checkBoxUser.TabIndex = 9;
            this.checkBoxUser.UseVisualStyleBackColor = true;
            // 
            // checkBoxCommon
            // 
            this.checkBoxCommon.AutoSize = true;
            this.checkBoxCommon.Checked = true;
            this.checkBoxCommon.CheckState = System.Windows.Forms.CheckState.Checked;
            this.checkBoxCommon.Location = new System.Drawing.Point(202, 85);
            this.checkBoxCommon.Name = "checkBoxCommon";
            this.checkBoxCommon.Size = new System.Drawing.Size(15, 14);
            this.checkBoxCommon.TabIndex = 10;
            this.checkBoxCommon.UseVisualStyleBackColor = true;
            // 
            // DumpDialog
            // 
            this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
            this.ClientSize = new System.Drawing.Size(257, 166);
            this.Controls.Add(this.checkBoxCommon);
            this.Controls.Add(this.checkBoxUser);
            this.Controls.Add(this.Inj);
            this.Controls.Add(this.label1);
            this.Controls.Add(this.lbl_message);
            this.Controls.Add(this.btn_cancel);
            this.Controls.Add(this.btn_ok);
            this.FormBorderStyle = System.Windows.Forms.FormBorderStyle.FixedDialog;
            this.Name = "DumpDialog";
            this.Text = "Dump request";
            this.ResumeLayout(false);
            this.PerformLayout();

        }

        #endregion

        private System.Windows.Forms.Button btn_ok;
        private System.Windows.Forms.Button btn_cancel;
        private System.Windows.Forms.Label lbl_message;
        private System.Windows.Forms.Label label1;
        private System.Windows.Forms.Label Inj;
        private System.Windows.Forms.CheckBox checkBoxUser;
        private System.Windows.Forms.CheckBox checkBoxCommon;
    }
}