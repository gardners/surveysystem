const path = require('path');
const fs = require('fs');

const { promisify } = require('util');

const _logfile_ = path.resolve('./log/player.log.csv');
let LOGFILE = _logfile_;

const appendFileAsync = promisify(fs.appendFile);

const SEPARATOR = ',';

const sanitize = function(val) {
    const type = typeof val;
    if (val === null || type === 'number' || type === 'boolean') {
        return '"' + val + '"';
    }
    if (type !== 'string') {
        return JSON.stringify(val);
    }
    return '"' + val.replace('"', '\'') + '"';
};

const init = function(filename = '') {
    if (filename) {
        LOGFILE = path.resolve(path.join('log', filename));
    }
    return appendFileAsync(LOGFILE, '')
        .then(() => LOGFILE);
};

const append = function(...args) {
    const line = args.map(val => sanitize(val)).join(SEPARATOR) + '\n';
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
