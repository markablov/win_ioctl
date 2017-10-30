const os = require('os');
const fs = require('fs');
const path = require('path');
const win_ioctl = require('../build/Release/win-ioctl');

const FSCTL_FILESYSTEM_GET_STATISTICS = 0x00090060;

const SIZE_OF_FILESYSTEM_STATISTICS = 0x38;

let filename = path.resolve(__filename);
console.log(`Get filesystem stats for ${filename}`);
fs.open(filename, 'r+', (err, fd) =>
{
    if (err)
    {
        console.log('Error: '+err);
        return;
    }
    win_ioctl(fd, FSCTL_FILESYSTEM_GET_STATISTICS, undefined, SIZE_OF_FILESYSTEM_STATISTICS, (err, data) =>
    {
        if (err)
        {
            if (err.code == 'ERANGE' && data)
            {
                let structSize = data.readUInt32LE(4);
                let fullData = win_ioctl(fd, FSCTL_FILESYSTEM_GET_STATISTICS, undefined, os.cpus().length * structSize);
                console.log('File has been read '+fullData.readUInt32LE(8)+' times');
            } else
                console.log('Error: '+err.message);
        } else
            console.log('Error: unexpected success of first request with small buffer!');
        fs.close(fd, ()=>{});
    });
});
