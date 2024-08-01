#define FUSE_USE_VERSION  26
#define _FILE_OFFSET_BITS 64

#include<stdio.h>
#include<time.h>
#include<sys/stat.h>
#include<stdbool.h>
#include<fuse.h>
#include<errno.h>
#include<string.h>
#include<stdlib.h>

char * get_name(const char* path);
char * get_parent(const char *path);
char * get_parent_path(const char *path);
int update(char  * name,char *parent,const char * path,struct stat * stbuf);
int is_dir(char* path);
void persistent();

#define MAX_NAME_SIZE 10
#define BLOCK_SIZE 512
#define MAX_PATH_LEN 100
#define MAX_BLOCK_SIZE 4096


typedef struct inode{
	int type;
	int inode_no;
	int size;
	int links_count;
	int blocks;
	time_t atime;//access_time
	time_t mtime;//modification time
	time_t ctime;//status change
	char name[MAX_NAME_SIZE];
	char path[MAX_PATH_LEN];
}inode;

typedef struct directory{
	char dir_name[MAX_NAME_SIZE];
	int inode_num;
	int num_files;
	int num_directories;
	char parent[MAX_NAME_SIZE];
	char parent_path[MAX_PATH_LEN];
	char path[MAX_PATH_LEN];
}directory;

typedef struct file{
	char file_name[MAX_NAME_SIZE];
	int inode_num;
	int size;
	int count;
	char parent[MAX_NAME_SIZE];
	char parent_path[MAX_PATH_LEN];
	char path[MAX_PATH_LEN];
	char filecontent[MAX_BLOCK_SIZE];
}file;

struct directory my_dir[20];
struct inode my_inode[100];
struct file my_file[80];
int curr_inode_no=0;
int curr_dir_no=0;
int curr_file_no=0;


static int map_inode_to_stat(struct inode *inode, struct stat *stbuf)
{
  if(inode == NULL || stbuf == NULL)
  {
    fprintf(stderr, "error: map_inode_to_stat reveiced null pointer\n");
    return -1;
  }
  stbuf->st_ino = inode->inode_no;
  stbuf->st_size = inode->size;
  stbuf->st_blksize = MAX_BLOCK_SIZE;
  stbuf->st_nlink = inode->links_count;
  stbuf->st_atime=inode->atime;
  stbuf->st_mtime=inode->mtime;
  stbuf->st_ctime=inode->ctime;
  stbuf->st_blocks=(inode->size/BLOCK_SIZE);
  stbuf->st_uid=getuid();
  stbuf->st_gid=getgid();
  if(inode->type == 1)
  {
    stbuf->st_mode = S_IFDIR | 0777;
  }
  else
  {
    stbuf->st_mode = S_IFREG | 0777;
    if(stbuf->st_size>0 && stbuf->st_size<=MAX_BLOCK_SIZE)
    	stbuf->st_blocks=8;
  

  }

  return 0;
}

static void* _init(struct fuse_conn_info * conn)
{
	printf("-----------------------File System Initialised------------------------\n");
	persistent();
	return 0;
}


static int _getattr(const char *path, struct stat * stbuf)
{
	printf("--------------------GETATTR-----------------------:path:%s\n",path);
	memset(stbuf, 0, sizeof(struct stat));
	int i;
	for(i=0;i<curr_inode_no;i++)
		{
			if(strcmp(my_inode[i].path,path)==0)
			{
				map_inode_to_stat(&my_inode[i],stbuf);
				return 0;
			}
		}
	return -ENOENT;
}

static int _readdir(const char *path, void *buffer, fuse_fill_dir_t filler,off_t offset, struct fuse_file_info *fi)
{
	printf("--------------------READDIR-----------------------:path:%s\n",path);
	int i=0;
	for(i=0;i<curr_inode_no;i++)
	{
		if(strcmp(my_inode[i].path,path)==0)
		{
			my_inode[i].atime=time(NULL);
			break;
		}
	}
	for(i=0;i<curr_file_no;i++)
	{
		if(strcmp(my_file[i].parent_path,path)==0)
		{
			filler(buffer,my_file[i].file_name,NULL,0);
		}
	}
	for(i=0;i<curr_dir_no;i++)
	{
		if(strcmp(my_dir[i].parent_path,path)==0)
		{
			filler(buffer,my_dir[i].dir_name,NULL,0);
		}
	}
	return 0;
return -ENOENT;
}

int _mkdir(const char *path, mode_t mode)
{
	printf("--------------------MKDIR-----------------------:path:%s\n",path);
	char *name=(char*)malloc(sizeof(char)*MAX_NAME_SIZE);
	char *parent=(char*)malloc(sizeof(char)*MAX_NAME_SIZE);
	name=get_name(path);
	parent=get_parent(path);
	int i;
	//printf("%s\n",get_p)
	for(i=0;i<curr_dir_no;i++)
	{
		if(strcmp(my_dir[i].path,path)==0 && strcmp(my_dir[i].parent_path,get_parent_path(path))==0)
		{
			return -EEXIST;
		}
	}

	strcpy(my_dir[curr_dir_no].dir_name,name);
	my_dir[curr_dir_no].inode_num=curr_inode_no;
	my_dir[curr_dir_no].num_files=0;
	my_dir[curr_dir_no].num_directories=0;
	my_inode[curr_inode_no].type=1;
	my_inode[curr_inode_no].inode_no=curr_inode_no;
	my_inode[curr_inode_no].size=MAX_BLOCK_SIZE;
	my_inode[curr_inode_no].links_count=2;
	my_inode[curr_inode_no].blocks=1;
	my_inode[curr_inode_no].atime=time(NULL);
	my_inode[curr_inode_no].ctime=time(NULL);
	my_inode[curr_inode_no].mtime=time(NULL);
	strcpy(	my_inode[curr_inode_no].name,name);
	strcpy(	my_dir[curr_dir_no].parent,parent);
	strcpy(	my_dir[curr_dir_no].parent_path,get_parent_path(path));
	strcpy(	my_dir[curr_dir_no].path,path);
	strcpy(my_inode[curr_inode_no].path,path);
	curr_inode_no++;
	curr_dir_no++;
	for(i=0;i<curr_dir_no;i++)
	{
		if(strcmp(my_dir[i].path,get_parent_path(path))==0)
		{
			my_dir[i].num_directories++;
			my_inode[my_dir[i].inode_num].links_count++;
			my_inode[my_dir[i].inode_num].mtime=time(NULL);
			my_inode[my_dir[i].inode_num].ctime=time(NULL);
			break;
		}
	}
	return 0;
}

int _rmdir(const char *path)
{
  	printf("--------------------RMDIR-----------------------:path:%s\n",path);
	int i,flag=0,j;
	printf("%s\n",get_parent_path(path));
	for(i=0;i<curr_file_no;i++)
	{
		if(strcmp(my_file[i].parent_path,path)==0)
		{
			return -ENOTEMPTY;
		}
	}
	for(i=0;i<curr_dir_no;i++)
	{
		if(strcmp(my_dir[i].parent_path,path)==0)
		{
			return -ENOTEMPTY;
		}
	}
	for(i=0;i<curr_dir_no;i++)
	{
		if(strcmp(my_dir[i].path,get_parent_path(path))==0)
		{
			my_dir[i].num_directories--;
			my_inode[my_dir[i].inode_num].links_count--;
			my_inode[my_dir[i].inode_num].mtime=time(NULL);
			my_inode[my_dir[i].inode_num].ctime=time(NULL);
			break;
		}
	}
    for(i=0;i<curr_dir_no;i++)
    {
    	if(strcmp(my_dir[i].path,path)==0)
    	{
        	flag=1;
     		for(j=i;j<curr_dir_no;j++)
      		{
      			my_dir[j]=my_dir[j+1];
      		}
      		curr_dir_no--;
      		break;
    	}
  	}
    for(i=0;i<curr_inode_no;i++)
    {
        if(strcmp(my_inode[i].path,path)==0)
        {
        	flag=1;
          	for(j=i;j<curr_inode_no;j++)
          	{
            	my_inode[j]=my_inode[j+1];
          	}
          	curr_inode_no--;
          	break;
        }
    }
	if(flag==1)
		return 0;
	else
		return -ENOENT;
}

int _mknod(const char *path, mode_t mode, dev_t dev)
{
	printf("--------------------MKNODE-----------------------:path:%s\n",path);
	int i,flag=0;
	char* name=(char*)malloc(sizeof(char)*MAX_NAME_SIZE);
	char* parent=(char*)malloc(sizeof(char)*MAX_NAME_SIZE);
	name=get_name(path);
	parent=get_parent(path);
	for(i=0;i<curr_dir_no;i++)
	{
		if(strcmp(get_parent_path(path),my_dir[i].path)==0)
		{      
			flag=1;
			break;
		}
	}
	if(flag==1)
	{
		strcpy(my_file[curr_file_no].file_name,name);
		my_file[curr_file_no].inode_num=curr_inode_no;
		my_file[curr_file_no].count=0;
		//my_file[curr_file_no].file_content=(char*)malloc(sizeof(char)*)
		strcpy(my_file[curr_file_no].filecontent,"");
		my_inode[curr_inode_no].type=0;
		my_inode[curr_inode_no].inode_no=curr_inode_no;
		my_inode[curr_inode_no].size=0;
		my_inode[curr_inode_no].links_count=1;
		my_inode[curr_inode_no].blocks=1;
		my_inode[curr_inode_no].atime=time(NULL);
		my_inode[curr_inode_no].ctime=time(NULL);
		my_inode[curr_inode_no].mtime=time(NULL);
		strcpy(my_inode[curr_inode_no].name,name);
		strcpy(my_file[curr_file_no].parent,parent);
		strcpy(my_file[curr_file_no].path,path);
		strcpy(my_file[curr_file_no].parent_path,get_parent_path(path));
		strcpy(my_inode[curr_inode_no].path,path);
		curr_inode_no++;
		curr_file_no++;
		for(i=0;i<curr_dir_no;i++)
		{
			if(strcmp(my_dir[i].path,get_parent_path(path))==0)
			{
				my_dir[i].num_files++;
				my_inode[my_dir[i].inode_num].links_count++;
				my_inode[my_dir[i].inode_num].mtime=time(NULL);
				my_inode[my_dir[i].inode_num].ctime=time(NULL);
				break;
			}
		}
		return 0;
	}
	else
	{
		return -ENOENT;
	}
}

int _utime(const char *path, struct utimbuf *ubuf)
{
    	printf("--------------------UTIME-----------------------:path:%s\n",path);
    	int i;
		for(i=0;i<curr_inode_no;i++)
		{
			if(strcmp(my_inode[i].path,path)==0)
			{
				my_inode[i].atime=time(NULL);
				my_inode[i].mtime=time(NULL);
				ubuf->actime=my_inode[i].atime;
				ubuf->modtime=my_inode[i].mtime;
				return 0;
			}
		}
		return -ENOENT;
}

static int _unlink(const char *path) 
{
	printf("--------------------UNLINK-----------------------:path:%s\n",path);
	if(is_dir(path))
		return -EISDIR;
	int i,flag,j;
		for(i=0;i<curr_dir_no;i++)
		{
			if(strcmp(my_dir[i].path,get_parent_path(path))==0)
			{
				//printf("%s\n",get_parent_path(path));
				my_dir[i].num_files--;
				my_inode[my_dir[i].inode_num].links_count--;
				my_inode[my_dir[i].inode_num].mtime=time(NULL);
				my_inode[my_dir[i].inode_num].ctime=time(NULL);
			}
		}
  	for(i=0;i<curr_file_no;i++)
  	{
    	if(strcmp(my_file[i].path,path)==0)
    	{
    		flag=1;
      		for(j=i;j<curr_file_no;j++)
      		{
      		  	my_file[j]=my_file[j+1];
      		}
      		curr_file_no--;
      		break;
    	}
  	}
    for(i=0;i<curr_inode_no;i++)
      	{
        	if(strcmp(my_inode[i].path,path)==0)
        	{
          		flag=1;
          		for(j=i;j<curr_inode_no;j++)
          		{
            		my_inode[j]=my_inode[j+1];
          		}
          		curr_inode_no--;
          		break;
        	}
      	}

		if(flag==0)
			return -ENOENT;
		else
			return 0;
}

static int _open(const char *path, struct fuse_file_info * fi)
{
	printf("--------------------OPEN-----------------------:path:%s\n",path);
	int i,flag=0;
	for(i=0;i<curr_file_no;i++)
	{
		if(strcmp(my_file[i].path,path)==0)
		{
			flag=1;
			break;
		}
	}
	if(flag==1)
	{
		my_file[i].count++;
		return 0;
	}
	else
		return -ENOENT;
}

int _read(const char *path, char *buf, size_t size, off_t offset, struct fuse_file_info *fi)
{
	printf("--------------------READ-----------------------:path:%s\n",path);
	int i,j,len;
	for(i=0;i<curr_file_no;i++)
	{
		if(strcmp(my_file[i].path,path)==0)
		{
			break;
		}
	}
	my_inode[my_file[i].inode_num].atime=time(NULL);
	len = strlen(my_file[i].filecontent);
    if (offset >= len) 
    {
      	return 0;
    }
    memcpy(buf, my_file[i].filecontent + offset, size);
    buf[strlen(buf)]='\n';
    
    return size;
  }

  

static int _write(const char *path, const char *buf, size_t size, off_t offset, struct fuse_file_info *fi) 
{
	printf("--------------------WRITE-----------------------:path:%s\n",path);
	int i,j,len;
	int flag=0;
	for(i=0;i<curr_file_no;i++)
	{
		if(strcmp(my_file[i].path,path)==0)
		{
			break;
		}
	}
	for(j=0;j<curr_inode_no;j++)
    {
        if(strcmp(my_inode[j].path,path)==0)
        {
          break;
        }
    }
 	my_inode[i].atime=time(NULL);
	my_file[i].size = size + offset;
	my_inode[j].size=my_file[i].size;
	memset((my_file[i].filecontent) + offset, 0, size);
	memcpy((my_file[i].filecontent) + offset, buf, size);

    if(offset>0)
    {
		my_file[i].filecontent[strlen(buf)+offset-1]='\0';	
	}
	else
	{
		my_file[i].filecontent[strlen(buf)]='\0';
	}
	memset((char *)buf, 0, strlen(buf));
	
	my_inode[i].mtime=time(NULL);
	my_inode[i].ctime=time(NULL);
	return size;
}
	
		

int _truncate(const char *path, off_t size)
{
	printf("--------------------TRUNCATE-----------------------:path:%s\n",path);
	return 0;
}

void _destroy(void *private_data)
{
	printf("--------------------DESTROY-----------------------\n");
	FILE *inode,*dir,*file;
	int i,j,k;
	dir=fopen("dir.txt","wb+");
	file=fopen("file.txt","wb+");
	inode=fopen("inode.txt","wb+");
	fprintf(inode,"%d\n",curr_inode_no);
	for(i=0;i<curr_inode_no;i++)
	{
		fprintf(inode,"%d\n",my_inode[i].type);
		fprintf(inode,"%d\n",my_inode[i].inode_no);
		fprintf(inode,"%d\n",my_inode[i].size);
		fprintf(inode,"%d\n",my_inode[i].links_count);
		fprintf(inode,"%s\n",my_inode[i].name);
		fprintf(inode,"%d\n",my_inode[i].blocks);
		fprintf(inode,"%s\n",my_inode[i].path);
		
	}
	fprintf(dir,"%d\n",curr_dir_no);
	for(i=0;i<curr_dir_no;i++)
	{
		fprintf(dir,"%s\n",my_dir[i].dir_name);
		fprintf(dir,"%d\n",my_dir[i].inode_num);
		fprintf(dir,"%d\n",my_dir[i].num_files);
		fprintf(dir,"%d\n",my_dir[i].num_directories);
		fprintf(dir,"%s\n",my_dir[i].parent);
		fprintf(dir,"%s\n",my_dir[i].path);
		fprintf(dir,"%s\n",my_dir[i].parent_path);
	}
	fprintf(file,"%d\n",curr_file_no);
	for(i=0;i<curr_file_no;i++)
	{
		fprintf(file,"%s\n",my_file[i].file_name);
		fprintf(file,"%d\n",my_file[i].inode_num);
		fprintf(file,"%d\n",my_file[i].size);
		fprintf(file,"%d\n",my_file[i].count);
		fprintf(file,"%s\n",my_file[i].parent);
		fprintf(file,"%s\n",my_file[i].path);
		fprintf(file,"%s\n",my_file[i].parent_path);
		my_file[i].filecontent[strcspn(my_file[i].filecontent, "\n")] = 0;
		fprintf(file,"%s\n",my_file[i].filecontent);
	}
	fclose(inode);
	fclose(dir);
	fclose(file);
}

static struct fuse_operations oper=
{
	.init=_init,
	.getattr=_getattr,
	.readdir=_readdir,
	.mkdir=_mkdir,
	.rmdir=_rmdir,
	.mknod=_mknod,
	.utime=_utime,
	.unlink=_unlink,
	.read=_read,
	.write=_write,
	.open=_open,
	.truncate=_truncate,
	.destroy=_destroy,
};

int main(int argc, char *argv[]) {
    return fuse_main(argc, argv, &oper, NULL);
}

char * get_name(const char* path)
{
	int i,j;
	char* name=(char*)malloc(sizeof(char)*MAX_NAME_SIZE);
	if(strcmp("/",path)==0)
		return "root";
	for(i=strlen(path)-1;i>=0;i--)
	{
		if(path[i]=='/')
		break;
	}
	path=path+i+1;
  return path;
}

int is_dir(char* path)
{
	int i;
	for(i=0;i<strlen(path);i++)
	{
		if(path[i]=='.')
		{
			return 0;
		}
	}
	return 1;
}

char * get_parent_path(const char *path)
{
	int i,j,k;
	char* parent_path=(char*)malloc(sizeof(char)*MAX_PATH_LEN);
	for(i=strlen(path)-1;i>=0;i--)
	{
		if(path[i]=='/')
		{
			break;
		}

	}
	if(i==0)
	{
		return "/";
	}
	else
	{
		for(j=0;j<i;j++)
		{
			parent_path[j]=path[j];
		}
		parent_path[j]='\0';
		return parent_path;
	}
}

char * get_parent(const char *path)
{
	int i,j,k,l;
	char* parent=(char*)malloc(sizeof(char)*20);
	for(i=strlen(path)-1;i>=0;i--)
	{
		if(path[i]=='/')
		{
			j=i;
			if(j!=0)
			{
				for(k=j-1;k>=0;k--)
				{
					if(path[k]=='/')
					{
						break;
					}
				}
				break;
			}
			else
			k=j;
		}
	}
	if(k==j)
	{
		return "root";
	}
	else
	{
		l=0;
		for(i=k+1;i<j;i++)
		{
			parent[l]=path[i];
			l++;
		}
		parent[l]='\0';
	}
	return parent;
}

void persistent()
{
	FILE *inode,*dir,*file;
	int flag=0;
	if(inode=fopen("inode.txt","r"))
	{
		fclose(inode);
		flag=1;
	}
	if(flag==0)
	{
		strcpy(my_dir[curr_dir_no].dir_name,"root");
		my_dir[curr_dir_no].inode_num=curr_inode_no;
		my_dir[curr_dir_no].num_files=0;
		my_dir[curr_dir_no].num_directories=0;
		my_inode[curr_inode_no].type=1;
		my_inode[curr_inode_no].inode_no=curr_inode_no;
		my_inode[curr_inode_no].size=4096;
		my_inode[curr_inode_no].links_count=2;
		my_inode[curr_inode_no].blocks=1;
		my_inode[curr_inode_no].atime=time(NULL);
		my_inode[curr_inode_no].ctime=time(NULL);
		my_inode[curr_inode_no].mtime=time(NULL);
		strcpy(	my_inode[curr_inode_no].name,"root");
		strcpy(	my_dir[curr_dir_no].parent,"none");
		strcpy(	my_dir[curr_dir_no].path,"/");
		strcpy(my_inode[curr_inode_no].path,"/");
		strcpy(	my_dir[curr_dir_no].parent_path,"none");

		curr_inode_no++;
		curr_dir_no++;
		file=fopen("file.txt","wb+");
		fprintf(file,"0\n");
		fclose(file);
	}
	else
	{
		int i;
		char c[1000];
		dir=fopen("dir.txt","r");
		file=fopen("file.txt","r");
		inode=fopen("inode.txt","r");
		fgets(c,1000,inode);
		curr_inode_no=atoi(c);
		fgets(c,1000,dir);
		curr_dir_no=atoi(c);
		fgets(c,1000,file);
		curr_file_no=atoi(c);
		for(i=0;i<curr_inode_no;i++)
		{
			fgets(c,1000,inode);
			my_inode[i].type=atoi(c);
			fgets(c,1000,inode);
			my_inode[i].inode_no=atoi(c);
			fgets(c,1000,inode);
			my_inode[i].size=atoi(c);
			fgets(c,1000,inode);
			my_inode[i].links_count=atoi(c);
			fgets(c,1000,inode);
			c[strcspn(c, "\n")] = 0;
			strcpy(my_inode[i].name,c);
			fgets(c,1000,inode);
			my_inode[i].blocks=atoi(c);	
			fgets(c,1000,inode);
			c[strcspn(c, "\n")] = 0;
			strcpy(my_inode[i].path,c);
			
		}
		for(i=0;i<curr_dir_no;i++)
		{
			fgets(c,1000,dir);
			c[strcspn(c, "\n")] = 0;
			strcpy(my_dir[i].dir_name,c);
			fgets(c,1000,dir);
			my_dir[i].inode_num=atoi(c);
			fgets(c,1000,dir);
			my_dir[i].num_files=atoi(c);
			fgets(c,1000,dir);
			my_dir[i].num_directories=atoi(c);
			fgets(c,1000,dir);
			c[strcspn(c, "\n")] = 0;
			strcpy(my_dir[i].parent,c);
			fgets(c,1000,dir);
			c[strcspn(c, "\n")] = 0;
			strcpy(my_dir[i].path,c);
			fgets(c,1000,dir);
			c[strcspn(c, "\n")] = 0;
			strcpy(my_dir[i].parent_path,c);
		}
		for(i=0;i<curr_file_no;i++)
		{
			fgets(c,1000,file);
			c[strcspn(c, "\n")] = 0;
			strcpy(my_file[i].file_name,c);
			fgets(c,1000,file);
			my_file[i].inode_num=atoi(c);
			fgets(c,1000,file);
			my_file[i].size=atoi(c);
			fgets(c,1000,file);
			my_file[i].count=atoi(c);
			fgets(c,1000,file);
			c[strcspn(c, "\n")] = 0;
			strcpy(my_file[i].parent,c);
			fgets(c,1000,file);
			c[strcspn(c, "\n")] = 0;
			strcpy(my_file[i].path,c);
			fgets(c,1000,file);
			c[strcspn(c, "\n")] = 0;
			strcpy(my_file[i].parent_path,c);
			fgets(c,1000,file);
			c[strcspn(c, "\n")] = 0;
			strcpy(my_file[i].filecontent,c);
		}
		fclose(file);
		fclose(inode);
		fclose(dir);



	}
}