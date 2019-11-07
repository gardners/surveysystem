const path = require('path');
const fs = require('fs');

const { promisify } = require('util');

const _logfile_ = path.resolve('./log/player.log');
let LOGFILE = _logfile_;

const appendFileAsync = promisify(fs.appendFile);

const sanitize = function(val) {
    const type = typeof val;
    if (val === null || type === 'string' || type === 'number' || type === 'boolean') {
        return val;
    }

    return JSON.stringify(val);
};

const init = function(filename = '') {
    if (filename) {
        LOGFILE = path.resolve(path.join('log', filename));
    }
    return appendFileAsync(LOGFILE, '')
        .then(() => LOGFILE);
};

const append = function(...args) {
    const line = args.map(val => sanitize(val)) + '\n';
    return appendFileAsync(LOGFILE, line);
};

const finish = function() {
    const latest = path.resolve('./log/latest');
    // symlink log file to "latest" file, so it's easy to find
    try {
        fs.unlinkSync(latest);
    } catch (err) {
        /* nothing */
    }

    fs.symlinkSync(LOGFILE, latest);
    return Promise.resolve(latest);
};

module.exports = {
    init,
    append,
    finish,
};
