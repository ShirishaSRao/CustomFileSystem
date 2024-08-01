# File System with FUSE

## Objective
- Create a virtual file system with FUSE which runs on top of OS.
- Implement the required system calls.
- The file system should pass all the test cases.
- The file system should be persistent.

## Phase 1

### File System Layout
- We have a structure for inode, file, and directory.
- Inode structure has all the elements required by the stat buffer.
- File structure contains inode number, name, path, parent path, size, and its content.
- Directory structure contains inode number, name, parent path, path.
- We also have an inode array of size 100, with each element as an inode structure, indicating that we can have 100 objects in our file system.
- We have a file array of size 80 and a directory array of size 20, indicating we can have 80 files and 20 directories. Each array element is a file and directory structure respectively.
- Three variables `curr_inode_no`, `curr_dir_no`, `curr_file_no` are used to represent the number of inodes, files, and directories.
- Block size is 512 bytes.
- We have a shell executable file which runs the command to initialize the file system.
- For unmounting, we can call `fusermount` or `Ctrl + C` which unmounts the file system.

## Phase 2

### System Calls

1. **init**
   - Initializes the file system.
   - It calls the `persistent()` function which copies the metadata from the external files to inodes of our files and updates the current inode number.
   - If the file system is newly initialized it creates an inode for the root directory.

2. **getattr**
   - For each path accessed, this function is called and it maps the inode of that path to the stat buffer.
   - If the path is invalid it returns an error.
   - Mapping is done by calling a helper function which maps the inode to stat. It also calculates the number of blocks required.

3. **readdir**
   - This is called when we call the `ls` system call.
   - It checks if the path is valid; else it returns an error.
   - It updates the access time for the directory accessed.
   - It lists all the children nodes of the requested path.
   - It parses the inode array and checks if the inode parent path matches the requested path.
   - Then it fills the filler buffer with those names.

4. **mkdir**
   - Called while creating the directory.
   - Parse the inode array and check if the directory already exists or not.
   - If it exists return an error; else create a new inode.
   - The access, change, and modification times are set to the time the directory was created.
   - Increment the link count of the parent directory.
   - Update the `mtime` and `ctime` of the parent directory.

5. **rmdir**
   - Parse the inode array and check if the children of the requested directory are present.
   - If present, return an error; else remove the directory.
   - Remove the inode from the inode array.
   - Decrement the links count of the parent directory.
   - Update the `mtime` and `ctime` of the parent directory.

6. **mknod**
   - Parse the inode array and check if the file exists or not.
   - If present return an error; else create an inode.
   - Initialize size to 0 and the file content to "".
   - Initialize the `atime`, `ctime`, `mtime` to the current time.
   - Increment the link count of the parent directory.
   - Update the `mtime` and `ctime` of the parent directory.

7. **utime**
   - It updates the access time and modification time of the file requested.
   - It updates `utimbuf` with those values.

8. **unlink**
   - Parse the inode and check if the file exists.
   - If it exists, remove the inode from the inode array.
   - Decrement the link count of the parent directory.
   - Update the `mtime` and `ctime` of the parent directory.

9. **read**
   - Parse the inode array and get the index of the inode.
   - If the offset is greater than the size of the file, return 0.
   - Else copy the content of the file from the offset to the buffer and return the requested size.
   - Update the `atime` of the file requested.

10. **write**
    - Parse the inode array and get the index of the inode.
    - Clear the file content and copy the content from the buffer.
    - Update the size of the file to the new size.
    - Update the `atime`, `mtime`, and `ctime` of the requested file.

### Helper Functions

1. **get_name**
   - It extracts the name of the current path and returns it.
   - The name of the path is used to update the inode name.

2. **inode_to_stat**
   - Copies the inode values to the stat buffer whenever required.

3. **get_parent_path**
   - It gets the parent path to store in the inode.
   - The parent path is required to update the details of the parent whenever its children are modified.

## Phase 3

### Persistence
- Persistence is implemented using an external file.
- When you call the `Ctrl + C` command, the file system is unmounted. The `destroy()` function is called at this time and it copies the whole metadata of the file into the external file.

1. **destroy()**
   - Creates three files `inode.txt`, `file.txt`, `dir.txt` which store the metadata for inodes, files, and directories.
   - If the file already exists, it appends the newly made changes in these files.

2. **persistent()**
   - This is called while initializing the file system.
   - It copies all the metadata from the external metadata files into the inodes.
