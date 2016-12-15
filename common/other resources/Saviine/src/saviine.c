#include <stdio.h>
#include <string.h>
#include <malloc.h>
#include "common/kernel_defs.h"
#include "dynamic_libs/os_functions.h"
#include "dynamic_libs/fs_functions.h"
#include "dynamic_libs/aoc_functions.h"
#include "dynamic_libs/socket_functions.h"
#include "dynamic_libs/sys_functions.h"
#include "game/rpx_rpl_table.h"
#include "game/memory_area_table.h"
#include "utils/net.h"
#include "saviine.h"
#include "utils/logger.h"

#define TITLE_LOCATION_ODD          0
#define TITLE_LOCATION_USB          1
#define TITLE_LOCATION_MLC          2

#define BUFFER_SIZE                 0x400 * 101
#define BUFFER_SIZE_STEPS           0x200

#define DEBUG_LOG                   0

extern ReducedCosAppXmlInfo cosAppXmlInfoStruct;

//! setup some default IP
static u32 serverIpAddress = 0xC0A800B5;

static int socket_saviine = -1;
static int socket_log = -1;

void StartDumper()
{
    void *pClient = NULL;
    void *pCmd = NULL;

    do{
      pClient = malloc(FS_CLIENT_SIZE);
        if (!pClient)
            break;

        pCmd = malloc(FS_CMD_BLOCK_SIZE);
        if (!pCmd)
            break;


    log_printf("StartDumper\n");
    FSInit();
    FSInitCmdBlock(pCmd);

    FSAddClientEx(pClient, 0, -1);

    log_printf("StartDumper\n");
    cafiine_connect(&socket_saviine,serverIpAddress);
    cafiine_connect(&socket_log,serverIpAddress);
    log_printf("StartDumper\n");

    if(socket_saviine >= 0 && socket_log >= 0){
        handle_saves(pClient, pCmd,-1);
    }
    FSDelClient(pClient);

    }while(0);

    if(pCmd)
        free(pCmd);
    if(pClient)
        free(pClient);

    if(socket_saviine >= 0)
    {
        cafiine_disconnect(socket_saviine);
        socket_saviine = -1;
    }
    if(socket_log >= 0)
    {
        cafiine_disconnect(socket_log);
        socket_log = -1;
    }
}

void handle_saves(void *pClient, void *pCmd,int error){
	if(DEBUG_LOG) log_string(socket_log, "init", BYTE_LOG_STR);
	unsigned char slotNo;
	long id = getPesistentID(&slotNo);
	init_Save(slotNo);
	int mode;

	if(DEBUG_LOG)log_string(socket_log, "getting mode", BYTE_LOG_STR);
	if(getMode(socket_saviine,&mode)){
		if(id >= 0x80000000 && id <= 0x90000000){
			char savepath[20];
			__os_snprintf(savepath, sizeof(savepath), "/vol/save/%08x",id);
			if(mode == BYTE_MODE_D){
				log_string(socket_log, "dump mode!", BYTE_LOG_STR);
				dumpSavaData(pClient, pCmd,id,error);
			}else if(mode == BYTE_MODE_I){
				log_string(socket_log, "inject mode", BYTE_LOG_STR);
				injectSaveData(pClient,pCmd,id,error);
			}
		}
	}
}

void dumpSavaData(void *pClient, void *pCmd,long persistentID,int error){
	/*
		Allocate buffer for injection
	*/
	int buf_size = BUFFER_SIZE;
 	char * pBuffer;
	int failed = 0;
	do{
		buf_size -= BUFFER_SIZE_STEPS;
		if(buf_size < 0){
			log_string(socket_log, "error on buffer allocation", BYTE_LOG_STR);
			failed = 1;
			break;
		}
		pBuffer = (char *)memalign(0x40,buf_size);
		if(pBuffer) memset(pBuffer, 0x00, buf_size);
	}while(!pBuffer);

	if(!failed){
		int mask = 0;
		char buffer[60];
		__os_snprintf(buffer, sizeof(buffer), "allocated %d bytes",buf_size);
		log_string(socket_log, buffer, BYTE_LOG_STR);
  		if(saviine_start_dump(socket_saviine, persistentID,&mask)){
			if((mask & MASK_USER) == MASK_USER){
				char savepath[20];
				__os_snprintf(savepath, sizeof(savepath), "/vol/save/%08x",persistentID);
				log_string(socket_log, "dumping user savedata", BYTE_LOG_STR);
				if(dump_dir(pClient,pCmd,savepath,pBuffer,buf_size,error,50) == -1){
					log_string(socket_log, "error dumping user dir", BYTE_LOG_STR);
				}
			}
			if((mask & MASK_COMMON) == MASK_COMMON){
				char * commonDir = "/vol/save/common";
				log_string(socket_log, "dumping common savedata", BYTE_LOG_STR);
				if(dump_dir(pClient,pCmd,commonDir,pBuffer,buf_size,error,60) == -1){
					log_string(socket_log, "error dumping common dir (maybe the game has no common folder?)", BYTE_LOG_STR);
				}
			}

			log_string(socket_log, "done!", BYTE_LOG_STR);

			if(!saviine_end_dump(socket_saviine)) if(DEBUG_LOG) log_string(socket_log, "saviine_end_injection() failed", BYTE_LOG_STR);
			if(DEBUG_LOG) log_string(socket_log, "end of dump", BYTE_LOG_STR);
		}else{
			log_string(socket_log, "saviine_start_dump() failed", BYTE_LOG_STR);
		}

		if(DEBUG_LOG) log_string(socket_log, "free(pBuffer) coming next", BYTE_LOG_STR);
		free(pBuffer);
		if(DEBUG_LOG) log_string(socket_log, "free(pBuffer)", BYTE_LOG_STR);
	}
}

int dump_dir(void *pClient, void *pCmd, char *path, void * pBuffer, int size,int error, int handle){
	int dir_handle = handle;
	int my_handle = handle +1;
	int ret = 0;
	int final_result = 0;
	if ((ret = FSOpenDir(pClient, pCmd, path, &dir_handle, FS_RET_ALL_ERROR)) == FS_STATUS_OK){
		char buffer[strlen(path) + 30];
		__os_snprintf(buffer, sizeof(buffer), "open dir %s",path);
		log_string(socket_log, buffer, BYTE_LOG_STR);
		FSDirEntry dir_entry;
		while (FSReadDir(pClient,  pCmd, dir_handle, &dir_entry, FS_RET_ALL_ERROR) == FS_STATUS_OK && final_result == 0)
		{
			char full_path[strlen(path) + 1 + strlen(dir_entry.name) +1];
			__os_snprintf(full_path, sizeof(full_path), "%s/%s",path,dir_entry.name);

			if((dir_entry.stat.flag&FS_STAT_FLAG_IS_DIRECTORY) == FS_STAT_FLAG_IS_DIRECTORY){
				log_string(socket_log, "-> dir", BYTE_LOG_STR);

				if(dump_dir(pClient, pCmd,full_path,pBuffer,size,error,my_handle) == -1){
					log_string(socket_log, "error", BYTE_LOG_STR);
					final_result = -1;
				}
			}else{
				//DUMP
				ret = FSOpenFile(pClient,  pCmd, full_path, "r", &my_handle, error);
				if (ret >= 0) {
					__os_snprintf(buffer, sizeof(buffer), "dumping %s",dir_entry.name);
					log_string(socket_log, buffer, BYTE_LOG_STR);

					int ret2;

					int  my_ret = cafiine_send_handle(socket_saviine, full_path, my_handle);
					if(my_ret != -1){
					while ((ret2 = FSReadFile(pClient,  pCmd, pBuffer, 1, size, my_handle, 0, 0)) > 0)
							cafiine_send_file(socket_saviine, pBuffer, ret2, my_handle);
						cafiine_fclose(socket_saviine, &ret2, my_handle,1);
					}else{
						log_string(socket_log, "error on opening file on pc" , BYTE_LOG_STR);
						final_result = -1;
					}
					if((ret2 = FSCloseFile(pClient,  pCmd, my_handle, error)) < FS_STATUS_OK){
						__os_snprintf(buffer, sizeof(buffer), "error on FSOpenFile: %d",ret2);
						log_string(socket_log, buffer, BYTE_LOG_STR);
					}
				}else{
					__os_snprintf(buffer, sizeof(buffer), "error on FSOpenFile: %d",ret);
					log_string(socket_log, buffer, BYTE_LOG_STR);
					final_result = -1;
				}
			}
		}
		if(FSCloseDir(pClient,  pCmd, dir_handle, error) <  FS_STATUS_OK){
			if(DEBUG_LOG) log_string(socket_log, "error on FSCloseDir()", BYTE_LOG_STR);
		}
	}else{
		log_string(socket_log, "error on FSOpenDir()", BYTE_LOG_STR);
		final_result = -1;
	}
	return final_result;
}

/**************************
	Injection functions
**************************/

void injectSaveData(void *pClient, void *pCmd,long persistentID,int error){
	char logbuffer[255];
	/*
		Allocate buffer for injection
	*/
	int buf_size = BUFFER_SIZE;
 	char * pBuffer;
	int failed = 0;
	do{
		buf_size -= BUFFER_SIZE_STEPS;
		if(buf_size < 0){
			log_string(socket_log, "error on buffer allocation", BYTE_LOG_STR);
			failed = 1;
			break;
		}
		pBuffer = (char *)memalign(0x40,buf_size);
		if(pBuffer) memset(pBuffer, 0x00, buf_size);
	}while(!pBuffer);

	if(!failed){
		char buffer[60];
		__os_snprintf(buffer, sizeof(buffer), "allocated %d bytes",buf_size);
		log_string(socket_log, buffer, BYTE_LOG_STR);
		int result = 0;
		int mask = 0;
		if((result = saviine_start_injection(socket_saviine, persistentID,&mask))){
			if((mask & MASK_USER) == MASK_USER){
				char savepath[20];
				__os_snprintf(savepath, sizeof(savepath), "/vol/save/%08x",persistentID);
				__os_snprintf(logbuffer, sizeof(logbuffer), "injecting new userdata in %08x",persistentID);
				log_string(socket_log, logbuffer, BYTE_LOG_STR);
				log_string(socket_log, "deleting user save", BYTE_LOG_STR);
				if(remove_files_in_dir(pClient,pCmd,savepath,0) == 0){
					/*
					Inject Save
					*/
					result = injectFiles(pClient,pCmd,savepath,"/",savepath,pBuffer,buf_size,error);
					doFlushOrRollback(pClient,pCmd,result,savepath);
				}
			}
			if((mask & MASK_COMMON) == MASK_COMMON && !failed){
				char * commonDir = "/vol/save/common";

				if((mask & MASK_COMMON_CLEAN) == MASK_COMMON_CLEAN){
					log_string(socket_log, "deleting common save", BYTE_LOG_STR);
					if(remove_files_in_dir(pClient,pCmd,commonDir,0) == -1){
						failed = 1;
					}
				}
				if(!failed){
					/*
					Inject common
					*/
					result = injectFiles(pClient,pCmd,commonDir,"/",commonDir,pBuffer,buf_size,error);
					doFlushOrRollback(pClient,pCmd,result,commonDir);
				}
			}
			if(!saviine_end_injection(socket_saviine)) if(DEBUG_LOG) log_string(socket_log, "saviine_end_injection() failed", BYTE_LOG_STR);
			if(DEBUG_LOG)log_string(socket_log, "end of injection", BYTE_LOG_STR);
		}else{
			log_string(socket_log, "saviine_start_injection() failed", BYTE_LOG_STR);
		}
		free(pBuffer);
	}
}

int injectFiles(void *pClient, void *pCmd, char * path,char * relativepath, char * basepath, char *  pBuffer, int buffer_size, int error){
	int failed = 0;
	int filesinjected = 0;
	int type = 0;
	log_string(socket_log, "injecting files", BYTE_LOG_STR);
	char namebuffer[255];
	char logbuffer[255];
	int filesize = 0;

	if(!failed){
		while(saviine_readdir(socket_saviine,path,namebuffer, &type,&filesize) && !failed){
			if(DEBUG_LOG)log_string(socket_log, "got a file", BYTE_LOG_STR);
			char newpath[strlen(path) + 1 + strlen(namebuffer) + 1];
			__os_snprintf(newpath, sizeof(newpath), "%s/%s",path,namebuffer);
			if(type == BYTE_FILE){
				__os_snprintf(logbuffer, sizeof(logbuffer), "file: %s%s size: %d",relativepath,namebuffer,filesize);
				log_string(socket_log, logbuffer, BYTE_LOG_STR);

				__os_snprintf(logbuffer, sizeof(logbuffer), "newpath: %s ",newpath);
				log_string(socket_log, logbuffer, BYTE_LOG_STR);

				if(DEBUG_LOG) log_string(socket_log, "downloading it", BYTE_LOG_STR);

				int handle = 10;
				if(FSOpenFile(pClient, pCmd, newpath,"w+",&handle,error) >= FS_STATUS_OK){
					if(DEBUG_LOG) log_string(socket_log, "file opened and created", BYTE_LOG_STR);

					if(filesize > 0){
						failed = doInjectForFile(pClient,pCmd,handle,newpath,filesize,basepath,pBuffer,buffer_size);
						if(failed == 2) // trying it again if the journal was full
							failed = doInjectForFile(pClient,pCmd,handle,newpath,filesize,basepath,pBuffer,buffer_size);
					}else{
						if(DEBUG_LOG) log_string(socket_log, "filesize is 0", BYTE_LOG_STR);
					}

					if((FSCloseFile (pClient, pCmd, handle, error)) < FS_STATUS_OK){
						log_string(socket_log, "FSCloseFile failed", BYTE_LOG_STR);
						failed = 1;
					}
				}else{
					log_string(socket_log, "opening the file failed", BYTE_LOG_STR);
					failed = 1;
				}
				if(!failed) filesinjected++;
			}else if( type == BYTE_FOLDER){
				__os_snprintf(logbuffer, sizeof(logbuffer), "dir: %s",namebuffer);
				log_string(socket_log, logbuffer, BYTE_LOG_STR);
				if(DEBUG_LOG) log_string(socket_log, newpath, BYTE_LOG_STR);
				int ret = 0;
				if((ret = FSMakeDir(pClient, pCmd, newpath, -1)) == FS_STATUS_OK || ret == FS_STATUS_EXISTS ){
					char op_offset[strlen(relativepath) + strlen(namebuffer)+ 1 + 1];
					__os_snprintf(op_offset, sizeof(op_offset), "%s%s/",relativepath,namebuffer);
					int injectedsub = injectFiles(pClient, pCmd, newpath,op_offset,basepath,pBuffer,buffer_size,error);
					if(injectedsub == -1){
						failed = 1;
					}else{
						filesinjected += injectedsub;
					}
				}else{
					log_string(socket_log, "folder creation failed", BYTE_LOG_STR);
					failed = 1;
				}
			}
		}
		if(failed) return -1;
		else return filesinjected;
	}else{
		return -1;
	}
}

int doInjectForFile(void * pClient, void * pCmd,int handle,char * filepath,int filesize, char * basepath,void * pBuffer,int buf_size){
	int failed = 0;
	int myhandle;
	int ret = 0;
	char logbuffer[255];
	if(DEBUG_LOG)__os_snprintf(logbuffer, sizeof(logbuffer), "cafiine_fopen %s",filepath);
    if(DEBUG_LOG) log_string(socket_log, logbuffer, BYTE_LOG_STR);

	if((cafiine_fopen(socket_saviine, &ret, filepath, "r", &myhandle)) == 0 && ret == FS_STATUS_OK){
		if(DEBUG_LOG)__os_snprintf(logbuffer, sizeof(logbuffer), "cafiine_fopen with handle %d",myhandle);
		if(DEBUG_LOG) log_string(socket_log, logbuffer, BYTE_LOG_STR);
			int retsize = 0;
			int pos = 0;
			while(pos < filesize){
				if(DEBUG_LOG) log_string(socket_log, "reading", BYTE_LOG_STR);
				if(cafiine_fread(socket_saviine, &retsize, pBuffer, buf_size , myhandle) == FS_STATUS_OK){
					if(DEBUG_LOG)__os_snprintf(logbuffer, sizeof(logbuffer), "got %d",retsize);
					if(DEBUG_LOG) log_string(socket_log, logbuffer, BYTE_LOG_STR);
					int fwrite = 0;
					if((fwrite = FSWriteFile(pClient, pCmd,  pBuffer,sizeof(char),retsize,handle,0,0x0200)) >= FS_STATUS_OK){
						if(DEBUG_LOG)__os_snprintf(logbuffer, sizeof(logbuffer), "wrote %d",retsize);
						if(DEBUG_LOG) log_string(socket_log, logbuffer, BYTE_LOG_STR);
					}else{
						if(fwrite == FS_STATUS_JOURNAL_FULL || fwrite == FS_STATUS_STORAGE_FULL){
							log_string(socket_log, "journal or storage is full, flushing it now.", BYTE_LOG_STR);
							if(FSFlushQuota(pClient,pCmd,basepath,FS_RET_ALL_ERROR) == FS_STATUS_OK){
								log_string(socket_log, "success", BYTE_LOG_STR);
								failed = 2;
								break;
							}else{
								log_string(socket_log, "failed", BYTE_LOG_STR);
								failed = 1;
								break;
							}

						}
						__os_snprintf(logbuffer, sizeof(logbuffer), "my_FSWriteFile failed with error: %d",fwrite);
						log_string(socket_log, logbuffer, BYTE_LOG_STR);
						//log_string(socket_log, "error while FSWriteFile", BYTE_LOG_STR);
						failed = 1;
						break;
					}
					if(DEBUG_LOG)__os_snprintf(logbuffer, sizeof(logbuffer), "old p %d new p %d",pos,pos+retsize);
					if(DEBUG_LOG) log_string(socket_log, logbuffer, BYTE_LOG_STR);
					pos += retsize;
				}else{
					log_string(socket_log, "error while recieving file", BYTE_LOG_STR);
					failed = 1;
					break;
				}
			}

			int result = 0;
			if((cafiine_fclose(socket_saviine, &result, myhandle,0)) == 0 && result == FS_STATUS_OK){
				if(DEBUG_LOG) log_string(socket_log, "cafiine_fclose success", BYTE_LOG_STR);
			}else{
				log_string(socket_log, "cafiine_fclose failed", BYTE_LOG_STR);
				failed = 1;
			}


	}else{
		log_string(socket_log, "cafiine_fopen failed", BYTE_LOG_STR);
		failed = 1;
	}
	return failed;
}

/*************************
	Util functions
**************************/

/*flush if result != -1*/

void doFlushOrRollback(void *pClient, void *pCmd,int result,char *savepath){
	char logbuffer[50 + strlen(savepath)];
	if(result != -1){
		__os_snprintf(logbuffer, sizeof(logbuffer), "injected %d files",result);
		log_string(socket_log, logbuffer, BYTE_LOG_STR);
		log_string(socket_log, "Flushing data now", BYTE_LOG_STR);
		if(FSFlushQuota(pClient,pCmd,savepath,FS_RET_ALL_ERROR) == FS_STATUS_OK){
			log_string(socket_log, "success", BYTE_LOG_STR);
		}else{
			log_string(socket_log, "failed", BYTE_LOG_STR);
		}
	}else{
		log_string(socket_log, "injection failed, trying to restore the data", BYTE_LOG_STR);
		if(FSRollbackQuota(pClient,pCmd,savepath,FS_RET_ALL_ERROR) == FS_STATUS_OK){
			log_string(socket_log, "rollback done", BYTE_LOG_STR);
		}else{
			log_string(socket_log, "rollback failed", BYTE_LOG_STR);
		}
	}
}

void init_Save(unsigned char slotNo){
	int (*SAVEInit)();
	int (*SAVEInitSaveDir)(unsigned char accountSlotNo);
	unsigned int save_handle;
	OSDynLoad_Acquire("nn_save.rpl", &save_handle);
	OSDynLoad_FindExport(save_handle, 0, "SAVEInit", (void **)&SAVEInit);
	OSDynLoad_FindExport(save_handle, 0, "SAVEInitSaveDir", (void **)&SAVEInitSaveDir);
	SAVEInit();
	if(DEBUG_LOG) log_string(socket_log, "saveinit done", BYTE_LOG_STR);
	SAVEInitSaveDir(slotNo);
	if(DEBUG_LOG) log_string(socket_log, "SAVEInitSaveDir done", BYTE_LOG_STR);
	SAVEInitSaveDir(255U);
	if(DEBUG_LOG) log_string(socket_log, "SAVEInitSaveDir 2 done", BYTE_LOG_STR);
}

long getPesistentID(unsigned char * slotno){
	unsigned int nn_act_handle;
	unsigned long (*GetPersistentIdEx)(unsigned char);
	int (*GetSlotNo)(void);
	void (*nn_Initialize)(void);
	void (*nn_Finalize)(void);
	OSDynLoad_Acquire("nn_act.rpl", &nn_act_handle);
	OSDynLoad_FindExport(nn_act_handle, 0, "GetPersistentIdEx__Q2_2nn3actFUc", (void **)&GetPersistentIdEx);
	OSDynLoad_FindExport(nn_act_handle, 0, "GetSlotNo__Q2_2nn3actFv", (void **)&GetSlotNo);
	OSDynLoad_FindExport(nn_act_handle, 0, "Initialize__Q2_2nn3actFv", (void **)&nn_Initialize);
	OSDynLoad_FindExport(nn_act_handle, 0, "Finalize__Q2_2nn3actFv", (void **)&nn_Finalize);

	nn_Initialize(); // To be sure that it is really Initialized

	*slotno = GetSlotNo();

	long idlong = GetPersistentIdEx(*slotno);

	nn_Finalize(); //must be called an equal number of times to nn_Initialize
	return idlong;
}

int remove_files_in_dir(void * pClient,void * pCmd, char * path, int handle){
	int ret = 0;
	int my_handle = handle +1;
	char buffer[strlen(path) + 50];
	if ((ret = FSOpenDir(pClient, pCmd, path, &handle, FS_RET_ALL_ERROR)) == FS_STATUS_OK){
		__os_snprintf(buffer, sizeof(buffer), "remove files in dir %s",path);
		log_string(socket_log, buffer, BYTE_LOG_STR);
		FSDirEntry dir_entry;
		while (FSReadDir(pClient,  pCmd, handle, &dir_entry, FS_RET_ALL_ERROR) == FS_STATUS_OK)
		{
			char full_path[strlen(path) + 1 + strlen(dir_entry.name) +1];
			__os_snprintf(full_path, sizeof(full_path), "%s/%s",path,dir_entry.name);
			if((dir_entry.stat.flag&FS_STAT_FLAG_IS_DIRECTORY) == FS_STAT_FLAG_IS_DIRECTORY){
				if(DEBUG_LOG) log_string(socket_log, "recursive deletion", BYTE_LOG_STR);
				if(remove_files_in_dir(pClient,pCmd,full_path,my_handle) == -1) return -1;

			}
			char buffer[strlen(full_path) + 50];
			__os_snprintf(buffer, sizeof(buffer), "deleting %s",full_path);
			log_string(socket_log, buffer, BYTE_LOG_STR);
			if((ret = FSRemove(pClient,pCmd,full_path,FS_RET_ALL_ERROR)) < FS_STATUS_OK){
				__os_snprintf(buffer, sizeof(buffer), "error: %d on removing %s",ret,full_path);
				log_string(socket_log, buffer, BYTE_LOG_STR);
				return -1;
			}

		}
		if((FSCloseDir(pClient,  pCmd, handle, FS_RET_NO_ERROR)) < FS_STATUS_OK ){
			log_string(socket_log, "error while closing dir", BYTE_LOG_STR);
			return -1;
		}
	}else{
		__os_snprintf(buffer, sizeof(buffer), "error: %d on opening %s",ret,path);
		log_string(socket_log, buffer, BYTE_LOG_STR);
		return -1;
	}
	return 0;
}

void ResetDumper(void)
{
    socket_saviine = -1;
    socket_log = -1;

    // reset RPX table on every launch of system menu
    rpxRplTableInit();
}

void SetServerIp(u32 ip)
{
    serverIpAddress = ip;
}

u32 GetServerIp(void)
{
    return serverIpAddress;
}
