# Driver communications
## Commands

### Common commands

---
#### OBOS_SERVICE\_GET\_SERVICE\_TYPE<br>
- Gets the type of the driver.
- This command takes no parameters
- Returns `enum serviceType`
- Can be accessed by anyone.

---
### OBOS_SERVICE\_TYPE\_FILESYSTEM
#### OBOS_SERVICE\_QUERY\_FILE\_DATA
- Queries file properties
- Parameters:<br>
&nbsp;&nbsp;&nbsp;sizeFilepath - SIZE\_T<br>
&nbsp;&nbsp;&nbsp;filepath - STRING<br>
&nbsp;&nbsp;&nbsp;driveId - UINT64\_T<br>
&nbsp;&nbsp;&nbsp;partitionId - UINT8\_T<br>
- Returns the file size in bytes, the LBA offset of the file, and `enum fileExistsReturn`
- Can only be accessed by the kernel.
#### OBOS\_SERVICE\_MAKE\_FILE\_ITERATOR
- Creates a file iterator.
- Parameters:<br>
&nbsp;&nbsp;&nbsp;driveId - UINT64\_T<br>
&nbsp;&nbsp;&nbsp;partitionId - UINT8\_T<br>
- Returns an `HANDLE` that can be used with iterator commands.
- Can only be accessed by the kernel.
#### OBOS\_SERVICE\_NEXT\_FILE
- Gets the next file in the iterator.
- Parameters:<br>
&nbsp;&nbsp;&nbsp;iterator - HANDLE
- Returns the same as OBOS\_SERVICE\_QUERY\_FILE\_DATA.
- Can only be accessed by the kernel.
### OBOS\_SERVICE\_TYPE\_INITRD\_FILESYSTEM <- OBOS\_SERVICE\_TYPE\_FILESYSTEM
#### OBOS\_SERVICE\_READ\_FILE
- Reads the file specified by the file path parameter synchronously.
- Parameters:<br>
&nbsp;&nbsp;&nbsp;sizeFilepath - SIZE\_T<br>
&nbsp;&nbsp;&nbsp;filepath - STRING<br>
&nbsp;&nbsp;&nbsp;driveId - UINT64\_T<br>
&nbsp;&nbsp;&nbsp;partitionId - UINT8\_T<br>
- Returns the size of the data read, and the file data.
- Can only be accessed by the kernel.
### OBOS\_SERVICE\_TYPE\_STORAGE\_DEVICE, SERVICE\_TYPE\_VIRTUAL\_STORAGE\_DEVICE
#### OBOS\_SERVICE\_READ\_LBA
- Reads `nSectors` at the offset `lbaOffset` from `driverId`
- Parameters:<br>
&nbsp;&nbsp;&nbsp;driveId - UINT64\_T<br>
&nbsp;&nbsp;&nbsp;lbaOffset - UINT64\_T<br>
&nbsp;&nbsp;&nbsp;nSectors - SIZE\_T<br>
- Returns a BYTE array with a size of nSectors * 512.
- Can be accessed by anyone.
#### OBOS\_SERVICE\_WRITE\_LBA
- Writes `nSectors` at the offset `lbaOffset` to `driverId`
- Parameters:<br>
&nbsp;&nbsp;&nbsp;driveId - UINT64\_T<br>
&nbsp;&nbsp;&nbsp;lbaOffset - UINT64\_T<br>
&nbsp;&nbsp;&nbsp;nSectors - SIZE\_T<br>
&nbsp;&nbsp;&nbsp;data - BYTE[nSectors * 512]<br>
- Returns void.
- Can be accessed by anyone.
### OBOS\_SERVICE\_TYPE\_USER\_INPUT\_DEVICE, SERVICE\_TYPE\_VIRTUAL\_USER\_INPUT\_DEVICE
#### OBOS\_SERVICE\_READ\_CHARACTER
- Receives one byte from the device.
- This function takes no parameters.
- Returns the byte read, or `nul` if there are no bytes to read.
- Can be accessed anywhere.
### TODO: SERVICE\_TYPE\_GRAPHICS\_DEVICE, SERVICE\_TYPE\_MONITOR, SERVICE\_TYPE\_KERNEL\_EXTENSION
### OBOS\_SERVICE\_TYPE\_COMMUNICATION, OBOS\_SERVICE\_TYPE\_VIRTUAL\_COMMUNICATION
#### OBOS\_SERVICE\_CONFIGURE\_COMMUNICATION
- Configures communication on the device. This is driver specific and can do whatever it wants with the parameters.
- Parameters:<br>
&nbsp;&nbsp;&nbsp;szParameters - SIZE\_T<br>
&nbsp;&nbsp;&nbsp;parameters - BYTE[szParameters]<br>
- Returns a `HANDLE` to the connection.
- Can be accessed anywhere.
#### OBOS\_SERVICE\_RECV\_BYTE\_FROM\_DEVICE
- Receives one byte from the device.
- This function takes no parameters.
- Returns the byte read, or `nul` if there are no bytes to read.
- Can be accessed anywhere.
#### OBOS\_SERVICE\_SEND\_BYTE\_TO\_DEVICE
- Sends one byte to the device.
- Parameters:<br>
&nbsp;&nbsp;&nbsp;toSend - BYTE<br>
- This function returns a DWORD describing a driver-specific error code.
- Can be accessed anywhere.