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

let win_ioctl_promise = (fd, code, input, outSize) =>
    new Promise((resolve, reject) => win_ioctl(fd, code, input, outSize, (err, data) =>
    {
        if (err)
            reject(err);
        else
            resolve(data);
    }));

let filename = path.resolve(__filename);
console.log(`Get compression for ${filename}`);
fs.open(filename, 'r+', (err, fd) =>
{
    if (err)
    {
        console.log('Error: '+err);
        return;
    }
    let saved_comp;
    win_ioctl_promise(fd, FSCTL_GET_COMPRESSION, undefined, 2).then(data =>
        {
            saved_comp = data;
            let val = data.readUInt16LE(0);
            console.log('Current compression: ' + compressions[val])
            return win_ioctl_promise(fd, FSCTL_SET_COMPRESSION, Buffer.from([val == 0 ? 0x01 : 0x00, 0x00]), 0);
        })
    .then(() => win_ioctl_promise(fd, FSCTL_GET_COMPRESSION, undefined, 2))
    .then(data =>
        {
            console.log('New compression: ' + compressions[data.readUInt16LE(0)]);
            return win_ioctl(fd, FSCTL_SET_COMPRESSION, saved_comp, 0);
        })
    .then(() => win_ioctl_promise(fd, FSCTL_GET_COMPRESSION, undefined, 2))
    .then(data => console.log('Returned compression: ' + compressions[data.readUInt16LE(0)]))
    .catch(err => console.log(err))
    .then(() => fs.close(fd, ()=>{}));
});
