win_ioctl
==========

Node.js add-on to allow calls of DeviceIoControl() on Windows machines

Installation
------------

Install with `npm`:

``` bash
$ npm install win_ioctl
```

API
--------

### win_ioctl(fd, code, input, outBufSize, cb)
**Parameters**
- **fd**: `Integer` file descriptor, must be open.
- **code**: `Integer` Device specific control code.
- **input** *optional*: `Buffer` Input data to send to device
- **outBufSize** *optional*: `Integer` Size of output buffer to allocate
- **cb** *optional*: `Function` Callback to be called after operation finish
Signature is `function (err, data)` where **err** is instance of `Error` and **data** is `Buffer`

**Returns**: `Buffer` Allocated buffer of **outBufSize** size with data returned by device or undefined if **cb** is passed

**Throws**: Throws if **cb** is unset

Examples
--------

Get compression status of file
```
const comp_names =
{
    0: 'COMPRESSION_FORMAT_NONE',
    1: 'COMPRESSION_FORMAT_DEFAULT',
    2: 'COMPRESSION_FORMAT_LZNT1',
};

let comp = win_ioctl(fd, FSCTL_GET_COMPRESSION, undefined, 2);
console.log('Compression: ' + comp_names[comp.readUInt16LE(0)]);
```

Async variant:
```
const comp_names =
{
    0: 'COMPRESSION_FORMAT_NONE',
    1: 'COMPRESSION_FORMAT_DEFAULT',
    2: 'COMPRESSION_FORMAT_LZNT1',
};

win_ioctl(fd, FSCTL_GET_COMPRESSION, undefined, 2, (err, d) =>
    console.log(err ? err : 'Compression: ' + comp_names[d.readUInt16LE(0)]));
```

Read variable-length data in case of ERR_MORE_DATA driver return:
```
win_ioctl(fd, FSCTL_FILESYSTEM_GET_STATISTICS, undefined, SIZE_OF_FILESYSTEM_STATISTICS, (err, data) =>
{
    if (err)
    {
        if (err.code == 'ERANGE' && data)
        {
            let structSize = data.readUInt32LE(4), fullSize = os.cpus().length * structSize;
            let fullData = win_ioctl(fd, FSCTL_FILESYSTEM_GET_STATISTICS, undefined, fullSize);
            console.log('File has been read '+fullData.readUInt32LE(8)+' times');
        } else
            console.log('Error: '+err.message);
    } else
        console.log('Error: unexpected success of first request with small buffer!');
});
```
