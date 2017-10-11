const fs = require('fs');
const path = require('path');
const win_ioctl = require('../build/Release/win-ioctl');

const FSCTL_GET_COMPRESSION = 0x0009003C;
const FSCTL_SET_COMPRESSION = 0x0009C040;

const compressions =
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
    // check current compression status
    let old_comp = win_ioctl(fd, FSCTL_GET_COMPRESSION, undefined, 2);
    let val = old_comp.readUInt16LE(0);
    console.log('Current compression: ' + compressions[val]);
    // set new compression to opposite
    win_ioctl(fd, FSCTL_SET_COMPRESSION, Buffer.from([val == 0 ? 0x01 : 0x00, 0x00]), 0);
    // show new compression
    let new_comp = win_ioctl(fd, FSCTL_GET_COMPRESSION, undefined, 2);
    console.log('New compression: ' + compressions[new_comp.readUInt16LE(0)]);
    // return to old compression
    win_ioctl(fd, FSCTL_SET_COMPRESSION, old_comp, 0);
    // show returned compression
    let ret_comp = win_ioctl(fd, FSCTL_GET_COMPRESSION, undefined, 2);
    console.log('Returned compression: ' + compressions[ret_comp.readUInt16LE(0)]);
    fs.close(fd, ()=>{});
});
