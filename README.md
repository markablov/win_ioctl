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
- **input**: `Buffer` Input data to send to device *optional*
- **outBufSize**: `Integer` Size of output buffer to allocate *optional*
- **cb**: `Function` Callback to be called after operation finish *optional*
Signature is usual `function (err, data)` where **err** is instance of `Error` and **data** is `Buffer`

**Returns**: `Buffer` Allocated buffer of **outBufSize** size with data returned by device or undefined if **cb** is passed

**Throws**: Throws if **cb** is unset

Examples
--------

Get compression status of file

```
const fs = require('fs');
const path = require('path');
const win_ioctl = require('win_ioctl');

const FSCTL_GET_COMPRESSION = 0x0009003C;

const comp_names =
{
    0: 'COMPRESSION_FORMAT_NONE',
    1: 'COMPRESSION_FORMAT_DEFAULT',
    2: 'COMPRESSION_FORMAT_LZNT1',
};

let filename = path.resolve(__filename);
console.log(`Get compression for ${filename}`);
fs.open(filename, 'r+', (err, fd) =>
{
    if (err)
    {
        console.log('Error: '+err);
        return;
    }
    let comp = win_ioctl(fd, FSCTL_GET_COMPRESSION, undefined, 2);
    console.log('Compression: ' + comp_names[comp.readUInt16LE(0)]);
    fs.close(fd, ()=>{});
});

```

Async variant:

```
const fs = require('fs');
const path = require('path');
const win_ioctl = require('win_ioctl');

const FSCTL_GET_COMPRESSION = 0x0009003C;

const comp_names =
{
    0: 'COMPRESSION_FORMAT_NONE',
    1: 'COMPRESSION_FORMAT_DEFAULT',
    2: 'COMPRESSION_FORMAT_LZNT1',
};

let filename = path.resolve(__filename);
console.log(`Get compression for ${filename}`);
fs.open(filename, 'r+', (err, fd) =>
{
    if (err)
    {
        console.log('Error: '+err);
        return;
    }
    win_ioctl(fd, FSCTL_GET_COMPRESSION, undefined, 2, (err, data) =>
    {
        if (err)
            console.log(err);
        else
            console.log('Compression: ' + comp_names[data.readUInt16LE(0)]);
        fs.close(fd, ()=>{});
    });
});

```